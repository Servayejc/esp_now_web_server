#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

#include <time.h>
#include <SD.h>
#include "logger.h"
//#include "HQ.cpp"
#include "AsyncTCP.h"
#include "Config.h"
#include "Svr.h"
#include "global.h"
#include "Utils.h"
#include <FS.h>



int pingID = 0;
int send = 0;
int led = 0;

unsigned long start;
bool serverReset = false;

bool canHandle = true;

uint8_t broadcastAddressR[] = {0x84, 0xF3, 0xEB, 0x80, 0xEF, 0xDA};
uint8_t broadcastAddressX[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t lastSendAddress[6] = {0};



//esp_now_peer_info_t slave;
int chan = 11; //
int pingTime = 0;




void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  //Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}


struct_reset pairReset;
struct_pairing ping;
struct_message incomingReadings;
struct_message outgoingSetpoints;
//struct_message pairingData;
/*
void connected_to_ap(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.println("[+] Connected to the WiFi network");
}

void disconnected_from_ap(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.println("[-] Disconnected from the WiFi AP");
  WiFi.begin(ssid, password);
}

void got_ip_from_ap(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.print("[+] Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}*/

void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}



void setup()
{
  // Initialize Serial Monitor
  Serial.begin(74880);
  start = millis();
  Serial.println(start);
  Serial.println();
  
  
  

  // Set the device as a Station and Soft Access Point simultaneously

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  chan = WiFi.channel();
  #ifdef DEBUG_WIFI
    Serial.print("Server SOFT AP MAC Address:  ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("Server MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());
    WiFi.printDiag(Serial);
  #endif  

  // Start NTP 
  configTime(0,0,ntpServer);

  // Mount LittleFS
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  #ifdef DEBUG_LITTLEFS
    Serial.print("LittleFS Mounted Total: ");
    Serial.print(LittleFS.totalBytes()/1024);
    Serial.print(" Used: ");
    Serial.print(LittleFS.usedBytes()/1024);
    Serial.println(" bytes");  
  #endif
  delay(1000);
  initSD();
  #ifdef DEBUG_LITTLEFS
    Serial.print("Card Type : ");
    Serial.println(SD.cardType());
    Serial.print("SD Mounted Total: ");
    Serial.print(SD.totalBytes()/1024);
    Serial.print(" Used: ");
    Serial.print(SD.usedBytes()/1024);
    Serial.println(" bytes");  
  #endif
  
  // initialize ESPNOW
  initESP_NOW();

  // Initilaze MDNS (Bonjour) 
  while (!MDNS.begin(hostName))
  {
    Serial.println("Starting mDNS...");
    delay(1000);
  }
  Serial.println("MDNS started");
  MDNS.addService("http", "tcp", serverPort);

  //showDirectory();
  //copyLittleFStoSD();
  
  #ifdef DEBUG_DIRECTORY
    showDirectory();
  #endif
  
                  // load saved peers from peers.js 
    
  setTimezone(timeZoneString); 
  initESP_NOW();
  if (fillDevices("/Struct.json")){ 
    Serial.println("Devices Loaded");
    loadPeers();        //  todo add param to allow addPeerToESPNOW
  }
  
  Serial.println("Starting server");
  startServer(); 
  Serial.println("Server started");   

  addPeerToESPNOW(broadcastAddressX);
  serverReset = true;
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
  
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;     //1000
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS)
  {
    lastEventTime = millis();
    checkMem();
    #ifdef DEBUG_PING
      Serial.print("Ping ID = ");
      Serial.println(pingData.msgID);
      Serial.println();
    #endif  
    #ifdef DEBUG_TIME
       printTime();
    #endif
    //setActivePeers();
    processLogger();
  }
  
}
