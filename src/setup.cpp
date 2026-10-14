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
#include "JSValidator.h"
#include <Preferences.h>

  #define BitVal(data,y) ( (data>>y) & 1) 

int pingID = 0;
int send = 0;
int led = 0;

#define LED 15

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
    safeSerialPrintf("Setting Timezone to %s\n",timezone.c_str());
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
            safeSerialPrintf(" --> ");
            printMAC(hdr->addr1);     //receiver MAC
            safeSerialPrintf(" : ");
            safeSerialPrintf("%d\n", rssi);
          #endif
        }
    }
}

void getWiFiChannel(){
    // Reset all peers in case of channel change
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      safeSerialPrintf("Checking WiFi channel change...\n");
    }
    delay(1000);
    safeSerialPrintf("Current WiFi channel: %d\n", WiFi.channel());
    if (checkWiFiChannel()){
      sendResetToPeers();
    }
    WiFi.disconnect();
}

void setWiFi(){
    getWiFiChannel();
    //connect to WiFi for Server and Esp_Now for transmit and recieve
    WiFi.mode(WIFI_AP_STA);
    esp_wifi_set_promiscuous(true); 
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    // Set the device as a Station and Soft Access Point simultaneously
   
    IPAddress local_IP(192, 168, 0, 200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      safeSerialPrintf("Setting as a Wi-Fi Station...\n");
    }
    delay(1000);

    
    #ifdef DEBUG_WIFI
      safeSerialPrintf("Server SOFT AP MAC Address: %s\n", WiFi.softAPmacAddress().c_str());
      safeSerialPrintf("Server MAC Address: %s\n", WiFi.macAddress().c_str());
      safeSerialPrintf("Station IP Address: %s\n", WiFi.localIP().toString().c_str());
      safeSerialPrintf("Wi-Fi Channel: %d\n", WiFi.channel());
      WiFi.printDiag(Serial);
    #endif
    // hide AP on WiFi network, it's not a real AP
    chan = WiFi.channel();
    WiFi.softAP(ssid, password, chan, true);
}

void mountLittleFS(){
    if(!LittleFS.begin(true)){
        //showError(12);
        safeSerialPrintf("An Error has occurred while mounting LittleFS\n");
        return;
    }
    #ifdef DEBUG_LITTLEFS
        safeSerialPrintf("LittleFS Mounted Total: %d\n", LittleFS.totalBytes()/1024);
        safeSerialPrintf(" Used: %d\n", LittleFS.usedBytes()/1024);
    #endif
}

void startMDNS(){
  safeSerialPrintf("Starting mDNS...\n");  
  while (!MDNS.begin(hostName))
  {
      safeSerialPrintf(".");
      delay(1000);
    }
    MDNS.addService("http", "tcp", serverPort);
    safeSerialPrintf("MDNS started\n");
}

void setTime(){
    struct tm timeinfo;
    safeSerialPrintf("Setting up time\n");
    configTime(0,0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
    
    if(!getLocalTime(&timeinfo)){
      safeSerialPrintf("  Failed to obtain time\n");
    return;
    }
    safeSerialPrintf("  Got the time from NTP\n");
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
  showDebug(false);
  
  

  xSemaphore = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();
  
  start = millis();
  safeSerialPrintf("Starting : %d \n", start);
  safeSerialPrintf("Firmware Version: %s \n", VERSION_SHORT);
  
  pinMode(LED,OUTPUT);
  digitalWrite(LED,HIGH);
  delay(100);
  digitalWrite(LED,LOW);

  
  setWiFi();
   
  setTime();  

  mountLittleFS();

  

  LOG.initSD(); 
  
  //validateJSON();

  startMDNS();
  
 
  #ifdef DEBUG_DIRECTORY
    showDirectory();
  #endif



  // initialize ESPNOW 
  initESP_NOW();
  safeSerialPrintf("========================\n");
  // DB.fillDevices("/Struct.json");
 // DB.loadPeers();
  DB.init("/Struct.json", "/peersList.js");          // add code to delete peersList.js if 
  safeSerialPrintf("========================\n");
  safeSerialPrintf("Starting server\n");
  startServer(); 
  safeSerialPrintf("Server started %\n");

  LOG.printTime();
  

  //showDirectory();
  
  /*if (WiFi.localIP() == "192.168.0.200"){
    safeSerialPrintf("------------IP OK");
  } else {
    safeSerialPrintf("------------Restarting, bad IP");
    ESP.restart();
  }*/
  
 // DB.printPeersFromFile();
  if (!LOG.isPresent()){
    safeSerialPrintf("SD card not available/n");
  //  ESP.restart();
  }
  
}
