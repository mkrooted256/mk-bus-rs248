#ifdef ENDPOINT_H
#define ENDPOINT_H

#include "bus.h"
#include "crc8/crc8.h"

struct Endoint {
public:
  const int rx_timeout = 1000;
  const int state_error = -1; // dead until 'reset' command
  const int state_idle = 1; // ready to receive
  const int state_rx_head = 10;   // receiving head
  const int state_rx_body = 11;   // receiving body
  const int state_busy = 20; // busy executing command or routine
  const int state_tx = 30;   // tx in progress

  const int rescode_ok = 0;
  const int rescode_timeout = 1;
  const int rescode_crc = 2;

  const int blocking_listen = true;
  
  Bus bus;
  int state = state_idle;
  int result_code = 0;

  size_t buffer_len = 0;
  uint8_t buffer[128];
  
private:
  long watchdog = 0;
  uint8_t current_crc = 0;

  void input(uint8_t b) {
    buffer[buffer_len] = b;
    current_crc = crc8(b, current_crc);
    buffer_len++;
  }
  
  msg_head& gethead() {
    return ((msg_head*) buffer)* 
  }

  void process_msg() = 0; // TODO
  
public:
  void listen(bool blocking = false) {
    // Wait until data available / timeout
    if (blocking) {
      watchdog = millis();
      while (!bus.carrier.available())
       if (millis() - watchdog > rx_timemout) return;
    } 
    // Otherwise just check if there any data
    else if (!bus.carrier.available()) return;

    // Receive and consume byte
    switch (state) {
      // Setup new buffer, reset crc, waiting for head input
      case state_idle: 
      {
        if (bus.carrier.read() == frameStart) {
          state = state_rx_head;
          buffer_len = 0;
          current_crc = 0;
        } else {
          // Frame not detected
          return;
        }
      }
      
      // Read one of 4 bytes of head
      case state_rx_head: 
      {
        auto b = bus.carrier.read();
        input(b);
        if (buffer_len == HEAD_SIZE) state = state_rx_body;
      }

      // Read body byte. Finalize received msg if frameEnd.
      case state_rx_body: 
      {
        auto b = bus.carrier.read();
        // Just read to buffer
        if (b != frameEnd) {
          input(b);
        } 
        // Otherwise this is frame end -- check crc and end rx
        // CRC(MSG+crcbyte) == 0 if no errors
        // crcbyte was previous byte, so
        else {
          if (current_crc == 0) {
            // Start msg processing
            state = state_busy;
            process_msg();
          } else {
            // Corrupted message, resend required
            // TODO: send special code to sender
          }
        }
      }
    }
  }

  void send(msg_head head) {
    state = state_tx;
    uint8_t crc = crc8(head, HEAD_SIZE);
  }
  
  void send(msg_head head, uint8_t* data) {
    state = state_tx;
    uint8_t crc = crc8(head, HEAD_SIZE);
    crc = crc8(data, head.size, crc);
    
    bus.carrier.write(frameStart);
    bus.carrier.write(head);
    bus.carrier.write(data, head.size);
    bus.carrier.write(crc);
    bus.carrier.write(frameEnd);

    state = state_idle;
  }
}

#endif //ENDPOINT_H
