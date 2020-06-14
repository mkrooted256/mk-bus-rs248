#ifdef BUS_H
#define BUS_H

#include <SoftwareSerial.h>

#define busModeReceive LOW
#define busModeTransmit HIGH 
#define busBaud 9600

#define frameStart uint8_t(':')
#define frameEnd uint8_t('\n')

#define CODE_PING        10
#define CODE_PONG        11
#define CODE_BROADCAST   20
#define CODE_RESET       30
#define CODE_

#define CODE_REQUEST     100
#define CODE_RESPOND     101

/*
 * Frame
 * 
 * [ 0    1    2      3       4     ...  size+3  size+4  size+5 ] bytes
 * [':'][ code from   to   ][ body       body   ][ CRC8 ][ '\n' ]
 * 
 */
 

struct Bus {
  SoftwareSerial * carrier;
  bool mode = busModeReceive;
  int busmode_pin;

  void setmode(bool newmode) { 
    digitalWrite(busmode_pin, mode = newmode); 
  }
  
  void begin() {
    pinMode(busModePin, OUTPUT);
    setmode(mode);
    carrier->begin(busBaud);
  }
  
};

const unsigned int HEAD_SIZE = 3;
struct msg_head {
  uint8_t code;
  uint8_t from
  uint8_t to;
}


#endif // BUS_H
