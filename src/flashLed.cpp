#include <Arduino.h>
#include "TickTwo.h"

bool ledState;
byte ledPin = 2;
uint8_t code2 = 0;
uint8_t longPulses = 0;
uint8_t shortPulses = 0;

#define SHORT   600
#define LONG    2000
#define SPACE   500
#define END     5000

/* error codes
11 Wifi can not connect
12 SD not mounted
  

*/

void blink();
TickTwo timer(blink, 1);

/* the error code must contains 2 digits between 1 and 9 */



void showError(uint8_t code){
    code2 = code;
    longPulses = 2 * (code / 10);
    shortPulses = 2 * (code % 10);
    /*Serial.println(longPulses);
    Serial.println(shortPulses);*/

    pinMode(ledPin, OUTPUT);
    ledState = 1;
    digitalWrite(ledPin, ledState);  // led ON
    timer.start();
    /*Serial.println("Started");*/
}

void processBlink(){
  uint32_t interval = 0;   
  if (code2 > 0){
    timer.update();
    if (timer.counter() % 2 == 0) { 
        interval = SPACE;  
    } else {
        if (timer.counter() > longPulses){
            interval = SHORT; 
        } else {
            interval = LONG; 
        }    
    }
    if (timer.counter()  > longPulses + shortPulses - 1) { 
        interval = END;
    }
    if (timer.counter() > longPulses + shortPulses){
        timer.stop();
        digitalWrite(ledPin, ledState);
        ledState = !ledState;
        timer.start();
    }  
    timer.interval(interval);
   } else {
      timer.stop();
      ledState = 0;
      digitalWrite(ledPin, ledState);  // led OFF
  }  
}

void blink() {
  /*Serial.print(timer.counter());
  Serial.print("  ");
  Serial.println(timer.interval()/1000);*/  
  digitalWrite(ledPin, ledState);
  ledState = !ledState;
}

