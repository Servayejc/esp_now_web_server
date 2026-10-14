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
#include <Config.h>
#include "Svr.h"
#include "global.h"
#include "Utils.h"
#include <DataBase.h>
#include "version.h"
#include "setup.h"
#include <Preferences.h>
#include "Utils.h"




/* 
credentials.h must be placed in a directory  ../Lib/  
and contains the following code

*/
#define BitVal(data,y) ( (data>>y) & 1) 

void setup()
{
  doSetup();
  //sendMessageToWhatsapp("VESTA Server restarted");
  //sendLogJson();
}

void loop()
{ 
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;     
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS)
  {
    lastEventTime = millis();
    checkMem();
    LOG.processLogger();
        
    if(checkWiFiChannel() || serverReset){ 
       
      
      ESP.restart();
    }
  }
 }
