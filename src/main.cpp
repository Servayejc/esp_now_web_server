#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
//#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "Logger.h"
#include <Arduino.h>
#include <Wire.h>

#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

#include <time.h>
#include <SD.h>

//#include "HQ.cpp"
#include "AsyncTCP.h"
#include "Config.h"
#include "Svr.h"
#include "global.h"
#include "Utils.h"
#include <FS.h>
#include <flashLed.h>
#include <HTTPClient.h>

#define BitVal(data,y) ( (data>>y) & 1) 

int pingID = 0;
int send = 0;
int led = 0;


#define LED 2

HTTPClient http;

unsigned long start;
bool serverReset = false;
SemaphoreHandle_t xSemaphore;
Logger LOG;

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

void setup()
{
  // Initialize Serial Monitor
  Serial.begin(74880);
  start = millis();
  Serial.println(start);
  Serial.println();

  xSemaphore = xSemaphoreCreateBinary(); 
  
  pinMode(LED,OUTPUT);
  digitalWrite(LED,HIGH);
  delay(1000);
  digitalWrite(LED,LOW);
 
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  
  esp_wifi_set_promiscuous(true); 
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station...");
  }
  chan = WiFi.channel();
  // hide AP on WiFi network, it's not a real AP  
  WiFi.softAP(ssid, password, chan, true);

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
  delay(1000);
  LOG.initSD(); 
  
  
  // Initilaze MDNS (Bonjour) 
  while (!MDNS.begin(hostName))
  {
    Serial.println("Starting mDNS...");
    delay(1000);
  }
  Serial.println("MDNS started");
  MDNS.addService("http", "tcp", serverPort);
 
  #ifdef DEBUG_DIRECTORY
    showDirectory();
  #endif

  setTimezone(timeZoneString);

  // initialize ESPNOW 
  initESP_NOW();
  
  // read structure for all peers and all devices
  if (fillDevices("/Struct.json")){ 
    Serial.println("Devices Loaded");
    loadPeers();       
  }
  
  Serial.println("Starting server");
  startServer(); 
  Serial.println("Server started");   

  addPeerToESPNOW(broadcastAddressX);
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
  processBlink();
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;     
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
    LOG.processLogger();
  }
}
