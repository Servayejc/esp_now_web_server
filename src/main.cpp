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
#include <Arduino.h>
#include <Wire.h>

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

void getHQData(JsonArray events) {
    Serial.println("getHQData");
    WiFiClient client;
    HTTPClient http;
    http.begin(client, "http://ofsys.hydroquebec.com/T/OFSYS/SM3/375/2/S/F/8509/18499810/aPy66RR6.html"); 
   
    int httpCode = http.GET();     
    if (httpCode == 200){
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
 
  /*http.begin("http://ofsys.hydroquebec.com/T/OFSYS/SM3/375/2/S/F/8509/18499810/aPy66RR6.html");
  int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

  http.end();*/
  



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

 /* Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }
  Serial.println("");*/

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
  //initESP_NOW();

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
  

  //serverReset = true;
  
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

  //saveOnSD("/ABC", "ABC");
  
}
