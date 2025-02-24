
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <LittleFS.h>
#include "Logger.h"
#include <SD.h>
#include "global.h"
#include "Config.h"
#include "Utils.h"
#include "print.h"
#include "Svr.h"


AsyncWebServer server(serverPort);  
AsyncEventSource events("/events");
AsyncWebSocket ws("/ws");

PeerList Peers = {};

struct_message setpoints = {};
struct_pairing pairingData = {};

std::map<std::string, std::string> SuffixToLogType_Map;
std::map<std::string, std::string>::iterator SuffixToLogType_Map_it;
std::map<std::string, struct_LogTemp>::iterator it;

#define LED 2

// ESP_NOW message received from a peer
void ProcessDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  digitalWrite(LED,!digitalRead(LED));
  
  char keyStr[30]; 
 
  StaticJsonDocument<1000> root;
  String payload;
  uint8_t type = incomingData[0]; // first message byte is the type of message
  int PeerID = macToPeerID(mac_addr);

#ifdef DEBUG_DATA_RECEIVED
   printlnMAC(mac_addr);
   Serial.println(PeerID);
#endif
  
  if (PeerID > 0)
  {
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
#ifdef DEBUG_DATA_RECEIVED
    printIncomingData();
    printDeviceAddress(incomingReadings.deviceAddress);
#endif
    
    char deviceStr[30];
    snprintf(deviceStr, sizeof(deviceStr), "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
           incomingReadings.deviceAddress[0], incomingReadings.deviceAddress[1],incomingReadings.deviceAddress[2], incomingReadings.deviceAddress[3], incomingReadings.deviceAddress[4], incomingReadings.deviceAddress[5], incomingReadings.deviceAddress[6], incomingReadings.deviceAddress[7]);
    
    // create a JSON document with received data and send it by event to the web page
    root["PeerID"] = PeerID;
    root["DeviceID"] = incomingReadings.deviceId;
    uint8_t deviceType = incomingReadings.deviceType;

#ifdef DEBUG_DATA_RECEIVED
    Serial.print("DeviceID : ");
    Serial.print(incomingReadings.deviceId);
#endif
    JsonArray D = root.createNestedArray("D");
    JsonObject Data = D.createNestedObject();
    char buffer[10];

#ifdef DEBUG_DATA_RECEIVED
    Serial.print("Sensor Address : ");
    for (byte i = 0; i < 8; i++)
    {
      Serial.print(incomingReadings.deviceAddress[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
#endif    
    Data["rssi"] = rssi;
    switch (deviceType)
    {
    case 0: // Thermostat
      snprintf(buffer, 10, "%2.2f", incomingReadings.F1);
      Data["F1"] = buffer;
      Data["F2"] = incomingReadings.F2;
      Data["U1"] = incomingReadings.U1;
      Data["MsgID"] = deviceStr;
      break;

    case 1: // Thermometer
      snprintf(buffer, 10, "%2.2f", incomingReadings.F1);
      Data["F1"] = buffer;
      Data["MsgID"] = deviceStr;
      break;

    case 2: // ON/OFF
      Data["Cmd"] = incomingReadings.U2;
      Data["U1"] = incomingReadings.U1;
      break;
    }
    
    //{"PeerID":3,"DeviceID":2,"D":[{"F1":"22.19","MsgID":"28:27:be:54:03:00:00:f2"}]}
    char keyStr[30];
    
    JsonObject Z = root["D"][0];   //{"F1":"22.19") 
    for (JsonPair kv : Z) {
      snprintf(keyStr, sizeof(keyStr), "%s_%d_%d",kv.key().c_str(),PeerID,incomingReadings.deviceId); // "F1_3_2"
      
      
      auto a = SuffixToLogType_Map.find((std::string)keyStr);

      if(a!= SuffixToLogType_Map.end()){
        if  (a->second == "L"){
          #ifdef DEBUG_DATA_RECEIVED  
            Serial.print("----logging ");
            Serial.print(a->first.c_str());
            Serial.print(" to ");
            Serial.print(a->second.c_str());
            Serial.print("  Value = ");
            Serial.println(kv.value().as<const char*>());
          #endif 
          addToLogData((std::string) a->first.c_str(), kv.value().as<float>());
        }
      }
    }
    serializeJson(root, payload);
    events.send(payload.c_str(), "new_readings", millis());
#ifdef DEBUG_DATA_RECEIVED
    Serial.print("event send :");
    serializeJson(root, Serial);
    Serial.println();
#endif
  }
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  uint8_t type = incomingData[0];
  switch (type)
  {
  case DATA:
    ProcessDataReceived(mac_addr, incomingData, len);
    break;

  case PING:
    memcpy(&pingData, incomingData, sizeof(pingData));
#ifdef DEBUG_PING
    Serial.print("Ping response from ");
    printMAC(mac_addr);
    Serial.print(" in ");
    Serial.print(millis() - pingTime);
    Serial.println(" ms ");
#endif
    break;

  case PAIRING: // the message is a pairing request
    memcpy(&pairingData, incomingData, sizeof(pairingData));
    #ifdef DEBUG_PAIRING
      Serial.print("Pairing request from: ");
      printlnMAC(mac_addr);
    #endif  
    StaticJsonDocument<1000> root;
    root["msgType"] = pairingData.msgType;
    root["network"] = pairingData.network; // ServerID
    root["id"] = pairingData.id;           // PeerID
    root["connected"] = 1;                 //
    root["channel"] = pairingData.channel;
    #ifdef DEBUG_PAIRING
      serializeJson(root, Serial);
      Serial.println();
    #endif
    if (pairingData.network == SERVER_ID) // reply only to network peers
    {
      if (pairingData.msgType == PAIRING)
      {
        //Serial.print(pairingData.id);
        //printlnMAC(mac_addr);
        addPeerToList(mac_addr, pairingData.id);
        // getDevices(pairingData.id);
        pairingData.id = SERVER_ID;
        // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
        WiFi.softAPmacAddress(pairingData.macAddr);
        pairingData.channel = chan;
        #ifdef DEBUG_PAIRING
          printPairingData();
        #endif  
        esp_now_send(mac_addr, (uint8_t *)&pairingData, sizeof(pairingData));
      }
    }
    break;
  }
}

void initESP_NOW()
{
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW initialyzed");
}

void sendSetPoints(int8_t peerId)
{
  uint8_t *mac_addr = PeerIDtoMAC(peerId);
  #ifdef DEBUG_SETPOINTS
    Serial.print("SendSetPoints to peer ");
    Serial.print(peerId);
    Serial.print(" mac = ");
    printlnMAC(mac_addr);
  #endif
  esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&setpoints, sizeof(setpoints));
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
#ifdef DEBUG_ON_SEND
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printlnMAC(mac_addr);
#endif
}

/*
  -------------------------------------
  ---------- WEB SOCKET ---------------
  -------------------------------------
*/
// Commands and SetPoints are received from client via web socket and
// transmitted to peer via ESP-NOW
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, int clientID)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    // {"peerID":"5","DevID":"3","DevType":"2","value":0}
    data[len] = 0;
    StaticJsonDocument<2000> root;

    auto error = deserializeJson(root, data);
    serializeJson(root, Serial);

    if (root["peerID"] == 99) {
      serverReset = true;
      return;
    }     

    setpoints.msgType = SETPOINTS;
    setpoints.deviceId = root["DevID"];
    setpoints.deviceType = getDeviceType(root["peerID"], root["DevID"]);
    setpoints.U1 = root["value"];

    #ifdef DEBUG_SETPOINTS
      Serial.println(setpoints.deviceId);
      Serial.println(setpoints.deviceType);
      Serial.println(setpoints.F1);
      Serial.println(setpoints.U1);
    #endif

    sendSetPoints(root["peerID"]);
  }
}

void onSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                   void *arg, uint8_t *ws_data, size_t len)
{
  #ifdef DEBUG_WS_EVENTS
    Serial.println("event");
  #endif  
  switch (type)
  {
  case WS_EVT_CONNECT:
    #ifdef DEBUG_WS_EVENTS
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    #endif 
    break;

  case WS_EVT_DISCONNECT:
    #ifdef DEBUG_WS_EVENTS
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
    #endif
    break;

  case WS_EVT_DATA:
    #ifdef DEBUG_WS_EVENTS
      Serial.printf("[WSc] get text: %s\n", ws_data);
    #endif  
    handleWebSocketMessage(arg, ws_data, len, client->id());
    break;

  case WS_EVT_PONG:
    break;
  case WS_EVT_ERROR:
    break;
  }
}

/*
  -------------------------------------
  ------------- SERVER ----------------
  -------------------------------------
*/

size_t load_data(File f, uint8_t* buffer, size_t maxLen, size_t index) {
    if (f.available()){  
      return f.read(buffer, maxLen);
    } else {
      return 0;
    } 
}


const char * html ="<p> %zxz% </p>";


String processor(const String& var) {
   
  if(var == "zxz"){
    return F("Hello world!");
  }
    
  if (var == "Config"){
    Serial.println("A");
    File peersFile = LittleFS.open("/peersList.js", "r");
    StaticJsonDocument<2000> root;
    DeserializationError error = deserializeJson(root, peersFile); 
    String result ;   
    serializeJsonPretty(root, result);
    String html;
    for (char ch : result){
      switch (ch){
      case 10:
        html += "<br>";
        //Serial.println("LF");
        break;
      
      case 32: 
        //Serial.println("SP");
        html += "&nbsp;&nbsp;&nbsp;&nbsp;";
        break;

      default:
        html += ch;
      }
    }
    return html;
  }

  return String();
}


void startServer()
{
  
 
  server.on("/readFile", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (xSemaphoreTake (xSemaphore, (50 * portTICK_PERIOD_MS))){
      int paramsNr = request->params(); 
      Serial.println(paramsNr);
      AsyncWebParameter * p = request->getParam(0); 
      Serial.println(p->value());
      
      #ifdef SERVER_TEST
        File f = LittleFS.open(p->value());
      #else
        //initSD();
        File f = SD.open(p->value(),"r",false);
      #endif  
      if (!f) {
        Serial.println ("Log file not found");
        request->send(404); //file not found
        return;
      } else {
        Serial.println("Log file opened");
          AsyncWebServerResponse* response = request->
          beginChunkedResponse("application/json", [f](uint8_t* buffer, size_t maxLen, size_t index)-> size_t{
          return load_data(f, buffer, 1024, index);
        });
        request->send(response);
      }
    } else {
      Serial.println("Log file Locked");
    }
    xSemaphoreGive (xSemaphore);
  });

  //server.on("/vest", HTTP_GET, [](AsyncWebServerRequest *request)
    //        { request->send(LittleFS, "/test.htm", String(), false, processor); });

  //server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
    //        { request->send(LittleFS, "/Config.htm"); }); 
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/server.htm"); }); 
  
  server.serveStatic("/", LittleFS, "/");   
  
  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->method() == HTTP_OPTIONS){
      request->send(200);
    }else{
      request->send(404);
    }   
  });

  // Web sockets
  server.addHandler(&ws);

  // Events
  ws.onEvent(onSocketEvent);
  events.onConnect([](AsyncEventSourceClient *client)
                   {
  if(client->lastId()){
     Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
  }
  
  // send event with message "hello!"
  // and set reconnect delay to 10 second
  client->send("hello!", NULL, millis(), 10000); });

  server.addHandler(&events);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  
  server.begin();

}

