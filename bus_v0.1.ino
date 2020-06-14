#include "bus.h"
#include "endpoint.h"
#include <SoftwareSerial.h>

#define busRx 8
#define busTx 9
#define busModePin 2


struct Led {
  int pin;
  bool mode;

  void set(bool newmode) { mode = newmode; digitalWrite(pin, mode); }
  void toggle() { mode = !mode; digitalWrite(pin, mode); }

  Led(pin=13, mode=LOW) : pin(pin), mode(mode) {
    pinMode(pin, OUTPUT);
    set(mode); 
  }
};

void indicate(Led& led, int n, int duration=200, int pause=duration) {
  for (int i=0; i<n; i++) {
    led.set(HIGH);
    delay(duration);
    led.set(LOW);
    delay(pause);
  }
}

/*
 * Frame
 * 
 * [ 0    1    2      3       4     ...  size+3  size+4  size+5 ] bytes
 * [':'][ code target size ][ body       body   ][ CRC8 ][ '\n' ]
 * 
 */

struct Slave : public Endpoint {
  
  
}

struct Master : BusPoint {
  void listen () = 0; // TODO
}

SoftwareSerial carrier(busRx,busTx);
Bus bus{carrier, busModeReceive, busModePin};
Led led();

void setup() {
  bus.begin();
  
  
}

void loop() {
  

}
