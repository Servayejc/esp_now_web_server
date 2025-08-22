#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <time.h>
#include <FS.h>

#include <flashLed.h>
#include "Logger.h"
#include "Config.h"
#include "Svr.h"
#include "global.h"
#include "Utils.h"
#include <DataBase.h>
#include "version.h"
#include "setup.h"

#define BitVal(data,y) ( (data>>y) & 1) 
#define LED 2

void setup()
{
  doSetup();
}

void loop()
{ 
  if (serverReset) {
    Serial.println("Pairing Request");
    pairReset.id = SERVER_ID;
    pairReset.msgType = RESET;
    esp_now_send(NULL, (uint8_t *) &pairReset, sizeof(pairReset));
    serverReset = false;
  } 
  //processBlink();
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;     
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS)
  {
    lastEventTime = millis();
    checkMem();
    LOG.processLogger();
  }
 }
