#include "Logger.h"
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <LittleFS.h>

#include <SD.h>
#include "global.h"
#include "Config.h"
#include "Utils.h"
#include "print.h"
#include "Svr.h"
#include "DataBase.h"
#include "Logger.h"

AsyncWebServer server(serverPort);  
AsyncEventSource events("/events");
AsyncWebSocket ws("/ws");

PeerList Peers = {};

struct_message setpoint = {};
struct_pairing pairingData = {};

std::map<std::string, std::string> SuffixToLogType_Map;
std::map<std::string, std::string>::iterator SuffixToLogType_Map_it;
std::map<std::string, struct_LogTemp>::iterator it;



// ESP_NOW message received from a peer
void ProcessDataReceived(const esp_now_recv_info* info, const uint8_t *incomingData, int len)
{

  uint8_t *mac_addr = info->src_addr;
  //digitalWrite(LED,!digitalRead(LED));
  
  char keyStr[30]; 
 
  StaticJsonDocument<1000> root;
  String payload;
  uint8_t type = incomingData[0]; // first message byte is the type of message
  int PeerID = DB.macToPeerID(mac_addr);

#ifdef DEBUG_DATA_RECEIVED
   printMAC(mac_addr);
   safeSerialPrintf(" - ",PeerID);
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
    safeSerialPrintf("DeviceID : %d\n",incomingReadings.deviceId);
#endif
    JsonArray D = root.createNestedArray("D");
    JsonObject Data = D.createNestedObject();
    char buffer[10];
#ifdef DEBUG_DATA_RECEIVED
    safeSerialPrintf("Sensor Address : ");
    for (byte i = 0; i < 8; i++)
    {
      safeSerialPrintf("%d ",incomingReadings.deviceAddress[i], HEX);
    }
    safeSerialPrintf("\n");
#endif    
    Data["rssi"] = rssi;
    switch (deviceType)
    {
    case THERMOSTAT: // Thermostat
      snprintf(buffer, 10, "%2.2f", incomingReadings.F1);
      Data["F1"] = buffer;
      Data["F2"] = incomingReadings.F2;
      Data["U1"] = incomingReadings.U1;
      Data["MsgID"] = deviceStr;
      break;

    case THERMOMETER: // Thermometer
      snprintf(buffer, 10, "%2.2f", incomingReadings.F1);
      Data["F1"] = buffer;
      Data["MsgID"] = deviceStr;
      break;

    case RELAY: // RELAY
      Data["Cmd"] = incomingReadings.U2;
      Data["U1"] = incomingReadings.U1;
      break;

    case LEVEL: // level
      snprintf(buffer, 10, "%f", incomingReadings.F1);
      Data["F1"] = buffer;
      sprintf (buffer, "%2.1f", (incomingReadings.F1*100)/8);
      Data["F2"] = buffer;
      Data["MsgID"] = deviceStr;
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
            safeSerialPrintf("----logging %s to %s value = %s\n", a->first.c_str(), a->second.c_str(), kv.value().as<const char*>());
           #endif 
          LOG.addToLogData((std::string) a->first.c_str(), kv.value().as<float>());
        }
      }
    }
    serializeJson(root, payload);
    events.send(payload.c_str(), "new_readings", millis());
#ifdef DEBUG_DATA_RECEIVED
    safeSerialPrintf("event send : ");
    serializeJson(root, Serial);
    safeSerialPrintf("\n");
#endif
  }
}

//typedef struct esp_now_recv_info {
//    uint8_t * src_addr;                      /**< Source address of ESPNOW packet */
//    uint8_t * des_addr;                      /**< Destination address of ESPNOW packet */
//    wifi_pkt_rx_ctrl_t * rx_ctrl;            /**< Rx control info of ESPNOW packet */
//} esp_now_recv_info_t;



void OnDataRecv(const esp_now_recv_info* info, const uint8_t *incomingData, int len) 
{
  //startTime();
  uint8_t type = incomingData[0];
  switch (type)
  {
  case DATA:
    if (len != sizeof(struct_message)) {
      safeSerialPrintf("***** Invalid DATA length *****\n");
      exit(0);
    }
    ProcessDataReceived(info, incomingData, len);
    break;

  case PING:
    if (len != sizeof(pingData)) return;
    memcpy(&pingData, incomingData, sizeof(pingData));
    pingData.msgType = PING;
    pingData.PeerID = SERVER_ID;
    esp_now_send(info->src_addr, (uint8_t *)&pingData, sizeof(pingData));
    /*#ifdef DEBUG_PING
        safeSerialPrintf("Ping response from ");
        printMAC(info->src_addr);
        safeSerialPrintf(" in %d ms \n", millis() - pingTime);
    #endif*/
    break;

  case PAIRING: // the message is a pairing request
  //if (len != sizeof(pairingData)) return; 
  //Serial.println(sizeof(pairingData));
  //Serial.println(len);
  memcpy(&pairingData, incomingData, sizeof(pairingData));
    #ifdef DEBUG_PAIRING
      safeSerialPrintf("Pairing request from: ");
      printMAC(info->src_addr);
      safeSerialPrintf("\n");
    #endif  
    StaticJsonDocument<1000> root;
    root["msgType"] = pairingData.msgType;
    root["network"] = pairingData.network; // ServerID
    root["id"] = pairingData.id;           // PeerID
    root["connected"] = 1;                 //
    root["channel"] = pairingData.channel;
    #ifdef DEBUG_PAIRING
      //serializeJson(root, Serial);
      //safeSerialPrintf("\n");
    #endif
    if (pairingData.network == SERVER_ID) // reply only to network peers
    {
      if (pairingData.msgType == PAIRING)
      {
        DB.addPeerToList(info->src_addr, pairingData.id);
        pairingData.id = SERVER_ID;
        // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
        WiFi.softAPmacAddress(pairingData.macAddr);
        pairingData.channel = chan;
        #ifdef DEBUG_PAIRING
          printPairingData();
        #endif  
        esp_now_send(info->src_addr, (uint8_t *)&pairingData, sizeof(pairingData));
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
    safeSerialPrintf("Error initializing ESP-NOW \n");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  safeSerialPrintf("ESP-NOW initialyzed \n");
}

void sendResetToPeers(){ 
    for (uint8_t i = 1; i < 12; i++) {
      uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, broadcastAddress, 6);
      peerInfo.channel = i;
      peerInfo.encrypt = false;

      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
          safeSerialPrintf("Broadcast peer added");
          esp_now_send(broadcastAddress, (uint8_t *) &pairReset, sizeof(pairReset));
          safeSerialPrintf("Reset message sent");
      }
    } 
  } 

/*void sendSetPoints(int8_t peerId)
{
  uint8_t *mac_addr = DB.PeerIDtoMAC(peerId);
  #ifdef DEBUG_SETPOINTS
    safeSerialPrintf("SendSetPoints to peer %d mac= ",peerId);
    printlnMAC(mac_addr);
  #endif
  esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&setpoints, sizeof(setpoints));
}*/
bool sendSetPoint(int8_t peerId, int8_t deviceId)
{
  uint8_t *mac_addr = DB.PeerIDtoMAC(peerId);
  #ifdef DEBUG_SETPOINTS
    printMAC(mac_addr);
  #endif
  esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&setpoint, sizeof(setpoint));
  if (result == ESP_OK) {
    safeSerialPrintf("SetPoint sent to peer %d deviceId %d value %d mac ",peerId, deviceId, setpoint.U1);
    printMAC(mac_addr);
    safeSerialPrintf("\n");
    DB.addSetPoint(peerId, deviceId, setpoint.U1);
    return true;
  } else { 
    return false;
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status != ESP_NOW_SEND_SUCCESS) {
    safeSerialPrintf("Delivery Fail to "); 
    printMAC(mac_addr);
    safeSerialPrintf("\n");
  }  

  #ifdef DEBUG_ON_SEND
  safeSerialPrintf("Last Packet Send Status: ");
  safeSerialPrintf(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
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

    setpoint.msgType = SETPOINTS;
    setpoint.deviceId = root["DevID"];
    setpoint.deviceType = DB.getDeviceType(root["peerID"], root["DevID"]);
    setpoint.U1 = root["value"];

    #ifdef DEBUG_SETPOINTS
      safeSerialPrintf("%d",setpoints.deviceId);
      safeSerialPrintf("%d",setpoints.deviceType);
      safeSerialPrintf("%d",setpoints.F1);
      safeSerialPrintf("%d",setpoints.U1);
    #endif

    if (!sendSetPoint(root["peerID"], root["DevID"])) {
      safeSerialPrintf("Failed to send set point to peer %d\n", root["peerID"]);
    } else {
      uint8_t *mac_addr = DB.PeerIDtoMAC(root["peerID"]);
      #ifdef DEBUG_SETPOINTS
        safeSerialPrintf("SendSetPoints to peer %d mac= ",peerId);
        printlnMAC(mac_addr);
      #endif
      esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&setpoint, sizeof(setpoint));
      if (result == ESP_OK) {
        DB.addSetPoint(root["peerID"], root["peerID"], root["value"]);
      }
    }  
  }
}

void onSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                   void *arg, uint8_t *ws_data, size_t len)
{
  #ifdef DEBUG_WS_EVENTS
    safeSerialPrintf("event");
  #endif  
  switch (type)
  {
  case WS_EVT_CONNECT:
    #ifdef DEBUG_WS_EVENTS
      safeSerialPrintf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    #endif 
    break;

  case WS_EVT_DISCONNECT:
    #ifdef DEBUG_WS_EVENTS
      safeSerialPrintf("WebSocket client #%u disconnected\n", client->id());
    #endif
    break;

  case WS_EVT_DATA:
    #ifdef DEBUG_WS_EVENTS
      safeSerialPrintf("[WSc] get text: %s\n", ws_data);
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
        break;
      
      case 32: 
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
    if( xSemaphoreTake( xSemaphore, (250 * portTICK_PERIOD_MS) == pdTRUE)){
      safeSerialPrintf("xSemaphore Take by Server\n");
      int paramsNr = request->params(); 
      if (request->hasParam("FN")){ 
        
        request->getParam("FN")->value();
        safeSerialPrintf("%d",request->getParam("FN")->value());
      }
      safeSerialPrintf("%d",request->getParam("FN")->value());
      
      #ifdef SERVER_TEST
        File f = LittleFS.open(request->getParam("FN")->value(),"r", false);
      #else
        //initSD();
        File f = SD.open(request->getParam("FN")->value(),"r",false);
      #endif  
      if (!f) {
        safeSerialPrintf("Log file not found");
        request->send(404); //file not found
        return;
      } else {
        safeSerialPrintf("Log file opened\n");
          AsyncWebServerResponse* response = request->
          beginChunkedResponse("application/json", [f](uint8_t* buffer, size_t maxLen, size_t index)-> size_t{
          return load_data(f, buffer, 1024, index);
        });
        request->send(response);
      }
    } else {
      safeSerialPrintf("Log file Locked");
    }
    xSemaphoreGive (xSemaphore);
    safeSerialPrintf("xSemaphore Give by Server\n");  
});

server.on("/updateFile", HTTP_GET, [](AsyncWebServerRequest* request) {
  int paramsNr = request->params(); 
  if (paramsNr == 2){
    if (request->hasParam("FN")){ 
      request->getParam("FN")->value();
      safeSerialPrintf(request->getParam("FN")->value().c_str());
    }
    if (request->hasParam("Data")){ 
      request->getParam("Data")->value();
      safeSerialPrintf(request->getParam("Data")->value().c_str());
    }
    LOG.saveOnSD(request->getParam("FN")->value(), request->getParam("Data")->value());  
    request->send(200);
  }
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
     safeSerialPrintf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
  }
  
  // send event with message "hello!"
  // and set reconnect delay to 10 second
  //client->send("hello!", NULL, millis(), 10000); 
  });
  
  server.addHandler(&events);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  
  server.begin();

}

