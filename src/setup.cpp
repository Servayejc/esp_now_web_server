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

#define BitVal(data,y) ( (data>>y) & 1) 

int pingID = 0;
int send = 0;
int led = 0;


#define LED 2

unsigned long start;
bool serverReset = false;
SemaphoreHandle_t xSemaphore;
Logger LOG;
DataBase DB;

int8_t rssi = 0;

uint8_t broadcastAddressR[] = {0x84, 0xF3, 0xEB, 0x80, 0xEF, 0xDA};
uint8_t broadcastAddressX[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t lastSendAddress[6] = {0};

//esp_now_peer_info_t slave;
int chan = 11; //
int pingTime = 0;

typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;


struct_reset pairReset;
struct_pairing ping;
struct_message incomingReadings;
struct_message outgoingSetpoints;



void setTimezone(String timezone){
    Serial.printf("Setting Timezone to %s\n",timezone.c_str());
    setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
}
  
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT)
        return;
    static const uint8_t ACTION_SUBTYPE = 0xd0;
    static const uint8_t ESPRESSIF_OUI[] = {0x84, 0xf3, 0xeb};
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    //Check if action frame has the Espressif OUI.
    if (ACTION_SUBTYPE == (hdr->frame_ctrl & 0xFF)){
        if (memcmp(hdr->addr2, ESPRESSIF_OUI, 3) == 0){
          rssi = ppkt->rx_ctrl.rssi;
          #ifdef DEBUG_RSSI
            printMAC(hdr->addr2);     //sender MAC
            Serial.print(" --> ");
            printMAC(hdr->addr1);     //receiver MAC
            Serial.print(" : ");
            Serial.println(rssi);
          #endif
        }
    }
}

void setWiFi(){
    WiFi.mode(WIFI_AP_STA);
    esp_wifi_set_promiscuous(true); 
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    // Set the device as a Station and Soft Access Point simultaneously
   
    IPAddress local_IP(192, 168, 0, 200);
    /*IPAddress gateway(192, 168, 0, 1);
    IPAddress subnet(255, 255, 0, 0);
    IPAddress primaryDNS(8, 8, 8, 8);   //optional
    IPAddress secondaryDNS(8, 8, 4, 4); //optional*/


    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("Setting as a Wi-Fi Station...");
    }
    delay(1000);
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
    // hide AP on WiFi network, it's not a real AP
    chan = WiFi.channel();
    WiFi.softAP(ssid, password, chan, true);
}

void mountLittleFS(){
    if(!LittleFS.begin(true)){
        showError(12);
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
}

void startMDNS(){
    Serial.println("Starting mDNS...");
    while (!MDNS.begin(hostName))
    {
        Serial.print(".");
        delay(1000);
    }
    MDNS.addService("http", "tcp", serverPort);
    Serial.println("MDNS started");
}

void setTime(){
    Serial.println("Setting time...");
    struct tm timeinfo;
    Serial.println("Setting up time");
    configTime(0,0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
    
    if(!getLocalTime(&timeinfo)){
      Serial.println("  Failed to obtain time");
    return;
    }
    Serial.println("  Got the time from NTP");
    // Now we can set the real timezone
    setTimezone(timeZoneString);

    printUtcTime();
    printLocalTime();


}


void doSetup()
{
  // Initialize Serial Monitor
  Serial.begin(74880);
  delay(1000);
  start = millis();
  Serial.println(start);
  Serial.println();
  Serial.print("Firmware Version: ");
  Serial.println(VERSION_SHORT);
 
  

  xSemaphore = xSemaphoreCreateMutex();
  
  pinMode(LED,OUTPUT);
  digitalWrite(LED,HIGH);
  delay(1000);
  digitalWrite(LED,LOW);
 
  setWiFi();
   
  setTime();  

  mountLittleFS();

  LOG.initSD(); 
  
  startMDNS();
 
  #ifdef DEBUG_DIRECTORY
    showDirectory();
  #endif



  // initialize ESPNOW 
  initESP_NOW();
  
  Serial.println("========================");
 // DB.fillDevices("/Struct.json");
 // DB.loadPeers();
  DB.init("/Struct.json", "/peersList.js");

  Serial.println("========================");

  Serial.println("Starting server");
  startServer(); 
  Serial.println("Server started");   

  LOG.printTime();
  //DB:addPeerToESPNOW(broadcastAddressX);
  //showDirectory();
  
  if (WiFi.localIP() == "192.168.0.200"){
    Serial.println("------------IP OK");
  } else {
    Serial.println("------------Restarting, bad IP");
    ESP.restart();
  }
  
  if (!LOG.isPresent()){
    Serial.println("SD card not available");
    ESP.restart();
  }
  
}
