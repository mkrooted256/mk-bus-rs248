#ifndef MK_BUS_RS248_MKBUS_H
#define MK_BUS_RS248_MKBUS_H

#include <Arduino.h>
#include <src/SoftwareSerial.h>

#define MKBUS_VERSION 0
#define ADDR_LENGTH   2
#define BUS_BAUD      9600
#define MSG_DELIM_LEN 4

namespace mkbus {
    typedef uint8_t bus_addr[ADDR_LENGTH];

    /* //----------- CONTAINERS ----------------\\ */
    struct Setup {
        bool     debug;
        uint8_t  pin_rx;
        uint8_t  pin_tx;
        bus_addr address;

        size_t   max_bufsize;
        uint32_t timeout;
    };
    struct msg {
        struct header {
            bus_addr address;
            uint8_t  opcode;
            uint64_t body_len;
            static constexpr size_t head_len = ADDR_LENGTH + 5;
        };

        header   head;
        uint8_t *body;
        uint16_t crc;

        uint16_t extract_crc() {
            crc =  body[head.body_len-3] << uint8_t(8);
            crc += body[head.body_len-2];
            return crc;
        }

        msg() : head{0,0,0}, body(nullptr), crc(0) {}
        explicit msg(const header &head) : head(head), crc(0) {
            body = new uint8_t[head.body_len+1];
            body[head.body_len] = 0;
        }
        ~msg() {
            delete body;
        }
    };
    struct QueryResult {
        // Query Result Codes
        // Can be combined like res = QR_TIMEOUT | QR_BUF_EXH
        // LSB is 0 iff operation successful
        static const uint8_t QR_OK         =  0;  //
        static const uint8_t QR_TIMEOUT    =  3;  // Slave did not answer
        static const uint8_t QR_CRC_FAILED =  5;  //
        static const uint8_t QR_BUF_EXH    =  9;  // Buffer exhausted - body is too long
        static const uint8_t QR_TX_ERR     = 17;  // Unspecified tx error
        static const uint8_t QR_RX_ERR     = 33;  // Unspecified rx error

        uint8_t status;
        msg     content;
    };

    // still unused:
    class RegisterList {
        struct R {
            uint8_t  rid;
            uint64_t val;
            R *      next;

            R(uint8_t rid, uint64_t val);
            ~R();
        };

        R *    begin;
        R *    end;
        size_t len;
    public:
        RegisterList();
        R * push(R *);
        R * get(uint8_t rid);
        ~RegisterList();
    };
    struct Device {
        bus_addr     address;
        RegisterList cache;
        String       info;

        explicit Device(bus_addr address) {
            memcpy(Device::address, address, ADDR_LENGTH);
        }
    };
    /* \\----------- CONTAINERS ----------------// */


    /* //------------- REQUESTS --------------\\ */
    /* Query compilers and response parsers      */
    class Request {
    protected:
        msg::header header;
        virtual bool ready() const = 0;
        virtual uint8_t * compile() const = 0;
    public:
        virtual QueryResult * parse_response(QueryResult * res) = 0;

        explicit Request(bus_addr target, uint8_t opcode) : header{} {
            memcpy(header.address, target, ADDR_LENGTH);
            header.opcode = opcode;
            header.body_len = 0;
        }

        const msg::header &get_header() const { return header; }
        uint8_t * as_bytes() const {
            if (!ready()) return nullptr;
            return compile();
        }
    };

    /*
     * INFO [0x01]
     * - Get string info about device
     * - No body, only header and crc sum
     */
    struct res_INFO : QueryResult {
        char* info;
    };
    class req_INFO : Request {
        bool ready() const override { return true; }
        uint8_t *compile() const override { return nullptr; };
    public:
        explicit req_INFO(uint8_t *target) : Request(target, 0x01) {}

    private:
        QueryResult *parse_response(QueryResult *in) override {
            auto * out = new res_INFO;
            out->status = in->status;
            out->content = in->content;
            out->info = (char*) in->content.body;
            return out;
        }
    };



    /* \\------------- REQUESTS --------------// */


    // --------------------------------------
    // ------------- MAIN CLASS -------------
    // --------------------------------------
    class Bus {
    public:
        enum State {
            ST_SETUP,  // During init
            ST_IDLE,   // Between rx, tx
            ST_TX,     //
            ST_AWAIT,  // Awaiting response after query
            ST_RX,     //
            ST_PROCESS // e.g. processing received data, calculating crc
        };
    private:
        uint32_t       timer=0;          // General timer. Used as watchdog
        State          state=ST_SETUP;   //
        Setup          setup;            // Bus init parameters
        SoftwareSerial serial;           //

        void        init();
        size_t      raw_send(uint8_t *data, size_t n, bool pause);  // Send byte sequence with guaranteed silence in front
        QueryResult raw_query(uint8_t *data, size_t n, bool pause); // Send byte seq. Validate and save answer in buffer
    public:
        static uint16_t CRC(const uint8_t* data, size_t n);

        explicit Bus(Setup setup);

        QueryResult * query(Request& req);
    };

}

#endif //MK_BUS_RS248_MKBUS_H
