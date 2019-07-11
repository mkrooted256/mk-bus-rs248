#include "mkbus.h"

#define DEBUG(ops) if (setup.debug) { ops }

using namespace mkbus;

// --------------------------------------
// ------------- MAIN CLASS -------------
// --------------------------------------
void Bus::init() {
    state = ST_SETUP;
    DEBUG(
            String str = "Setting up bus with address\n";
            for (unsigned char b : setup.address) {
                char *n = " 0x00";
                itoa(b, n + 3, 16);
                str += n;
            }
            str += "...";
            Serial.print(str);
    )
    serial.begin(BUS_BAUD);
    serial.listen();
    DEBUG(
            Serial.print("online at ");
            Serial.println(BUS_BAUD);
    )

    // TODO search
    // TODO init of the line

    DEBUG(
            Serial.println("Bus idle");
    )
    state = ST_IDLE;
    timer = millis();
}

Bus::Bus(Setup setup) : setup(setup), serial(setup.pin_rx, setup.pin_tx) {
    init();
}

size_t Bus::raw_send(uint8_t *data, size_t n, bool pause=true) {
    state = ST_TX;
    if (pause) {
        constexpr uint8_t delim[MSG_DELIM_LEN]{0};
        DEBUG(
                Serial.print("Writing ");
                Serial.print(n);
                Serial.print("bytes...");
        )
        serial.write(delim, MSG_DELIM_LEN);
    }
    size_t res = serial.write(data, n);
    serial.flush();
    DEBUG(
            Serial.println("idle");
    )
    state = ST_IDLE;

    return res;
}

QueryResult Bus::raw_query(uint8_t *data, size_t n, bool pause=true) {
    QueryResult result = {QueryResult::QR_OK};

    size_t bytes_sent;
    bytes_sent = raw_send(data, n, pause);
    if (bytes_sent != n) result.status |= QueryResult::QR_TX_ERR;
    DEBUG(Serial.print("ST_AWAIT...");)
    state = ST_AWAIT;
    timer = millis();

    // waiting for response
    while (!serial.available()) {
        if (millis() - timer >= setup.timeout) {
            state = ST_IDLE;
            return {QueryResult::QR_TIMEOUT};
        }
    }

    state = ST_RX;
    // Read message header: slave address, response code, body length
    msg::header rx_header{};
    serial.readBytes((uint8_t*)&rx_header, msg::header::head_len);
    if (rx_header.body_len > setup.max_bufsize) {
        result.status |= QueryResult::QR_BUF_EXH;
    }

    // Read message body. Note that real msg len is body_len+2 due to crc
    result.content = msg(rx_header);
    size_t bytes_received =
            serial.readBytes(result.content.body, result.content.head.body_len + 2);
    state = ST_PROCESS;
    if (bytes_received != result.content.head.body_len + 2) {
        result.status |= QueryResult::QR_RX_ERR;
    }
    // Check CRC - compare calculated crc of message (except last 2 bytes) with received crc
    result.content.extract_crc();
    if (result.content.crc != CRC(result.content.body, result.content.head.body_len)) {
        result.status |= QueryResult::QR_CRC_FAILED;
    }
    state = ST_IDLE;
    return result;
}


// TODO Rewrite this copied from google implementation of CRC16
uint16_t Bus::CRC(const uint8_t *data, size_t n) {
    const uint16_t generator = 0x8005;
    const uint16_t msb = uint16_t(1) << uint8_t(15);
    uint16_t remainder = 0;

    for (size_t b=0; b<n; b++) {
        remainder ^= (data[b] << uint8_t(8));
        for (uint8_t bit=8; bit>0; bit++) {
            remainder = (remainder << uint8_t(1));
            if (remainder & msb) remainder ^= generator;
        }
    }

    return remainder;
}

QueryResult * Bus::query(Request& req) { // TODO UNFINISHED
    // Compile and calculate crc
    uint8_t * body = req.as_bytes();
    uint16_t  crc  = CRC(body, req.get_header().body_len);
    // Send header
    uint8_t status_code =
            raw_send((uint8_t *) &req.get_header(), msg::header::head_len);
    // Send body
    status_code |= raw_send(body, msg::header::head_len, false);
    // Send CRC
    auto * result = new QueryResult;
    *result = raw_query((uint8_t*) &crc, 2, false);
    result->status |= status_code;

    result = req.parse_response(result);

    return result;
}

// -----------------------------------------
// ------------ CONTAINERS -----------------
// -----------------------------------------

RegisterList::R::R(uint8_t rid, uint64_t val) : rid(rid), val(val), next(nullptr) {}

RegisterList::R::~R() {
    delete next;
}

RegisterList::RegisterList() : begin(nullptr), end(nullptr), len(0) {}

RegisterList::R *RegisterList::push(RegisterList::R *reg) {
    if (!begin) {
        begin = end = reg;
    } else {
        end = end->next = reg;
    }
    len++;
    return end;
}

RegisterList::R *RegisterList::get(uint8_t rid) {
    R *r = begin;
    for (; r != nullptr; r = r->next) {
        if (r->rid == rid) break;
    }
    return r;
}

RegisterList::~RegisterList() {
    delete begin;
}

// ------------------------------------------
// ----------- REQUESTS ---------------------

