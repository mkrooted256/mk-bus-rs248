#include <Arduino.h>
#include "mkbus.h"

void setup()
{
    Serial.begin(9600);
    Serial.println("Helloworld; Version:");
    Serial.println(MKBUS_VERSION);

}

void loop()
{
    Serial.println("Br0ther, may i have some l00ps?");
    delay(5000);
}