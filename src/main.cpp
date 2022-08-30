/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp-now-wi-fi-web-server/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

/*
  JC Servaye 
  Complete project details at https://github.com/Servayejc/esp_now_web_server/
  added websocket access
  added auto pairing test

  ESP32 in WIFI_AP_STA mode respond whith his WiFi.macAddress but it uses WiFi.softAPmacAddress to receive from ESP8266 peer.
  example : 
      1.- The peer send a message to the server with address ff:ff:ff:ff:ff:ff 
      2.- The server receive the message and the address of the peer.
      3.- The server add the address of the peer to his peer list. 
      4.- The server reply to the peer whith the received address. 
      5.- The peer receive the message and the WiFi.macAddress of the server.
      6.- The peer add the received address of the server to his peer list.
      7.- The peer try to send a message to the server address but it fail to transmit !!!
      8.- The peer add the WiFi.softAPmacAddress of the server to his peer list.
      9.- The peer send a message to the server WiFi.softAPmacAddress... 
      10.- The server receive the message from the peer!
  
  Don't trust the macAddress in the OnDataRecv(...) callback if the message is received 
  from an ESP in WIFI_AP_STA mode.  
  
*/

#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>

int send = 0;
int led = 0;

uint8_t broadcastAddress[] =  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// Replace with your network credentials (STATION)
const char* ssid = "COGECO-BE360";
const char* password = "3FEADCOGECO";

esp_now_peer_info_t slave;
int chan = 11;  // 

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  uint8_t msgType;
  uint8_t id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

typedef struct struct_pairing {       // new structure for pairing
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;

 

struct_message incomingReadings;
struct_message outgoingSetpoints;
struct_pairing pairingData;


//JSONVar board;
//JSONVar ws_json;

AsyncWebServer server(80);
AsyncEventSource events("/events");
AsyncWebSocket ws("/ws");




const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4><p><span class="reading"><span id="t1"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #1 - HUMIDITY</h4><p><span class="reading"><span id="h1"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh1"></span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #2 - TEMPERATURE</h4><p><span class="reading"><span id="t2"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt2"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #2 - HUMIDITY</h4><p><span class="reading"><span id="h2"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh2"></span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>)rawliteral";

// web socket received setpoints, copy values to a structure  
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, int clientID) {
	  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      //  parseJson(data, clientID);
      StaticJsonDocument<2000> root;
      auto error  = deserializeJson(root, data);
      outgoingSetpoints.id = root["id"];
      outgoingSetpoints.temp = root["temp"];
      outgoingSetpoints.hum = root["hum"];
      outgoingSetpoints.readingId = root["readingId"];

      Serial.println(outgoingSetpoints.id);
      Serial.println(outgoingSetpoints.temp);
      Serial.println(outgoingSetpoints.hum);
      Serial.println(outgoingSetpoints.readingId);
    }
}

void onSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *ws_data, size_t len) {
    Serial.println("event");
	switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
	      break;

      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
 		    break;
      
	    case WS_EVT_DATA:
        Serial.printf("[WSc] get text: %s\n", ws_data);
        // transform JSON data received to a structure
        handleWebSocketMessage(arg, ws_data, len, client->id());
        // set a flag to transmit structure
        send = 1;
        break;      
        
      case WS_EVT_PONG:
        break;
      case WS_EVT_ERROR:
        break;
  }
}

// ---------------------------- esp_ now -------------------------
void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

bool addPeer(const uint8_t *peer_addr) {      // add pairing
  memset(&slave, 0, sizeof(slave));
  const esp_now_peer_info_t *peer = &slave;
	memcpy(slave.peer_addr, peer_addr, 6);
  
	slave.channel = chan; // pick a channel
  slave.encrypt = 0; // no encryption
  // check if the peer exists
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists) {
    // Slave already paired.
    Serial.println("Already Paired");
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK) {
      // Pair success
      Serial.println("Pair success");
      return true;
    }
    else 
    {
      Serial.println("Pair failed");
      return false;
    }
  }
} 

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}




void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  if (len == sizeof(incomingReadings)){
    Serial.print(len);
    Serial.print(" bytes of data received from : ");
    printMAC(mac_addr);
    Serial.println();
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    // create a JSON document with received data and send it by event to the web page
    StaticJsonDocument<1000> root;
    root["id"] = incomingReadings.id;
    root["temperature"] = incomingReadings.temp;
    root["humidity"] = incomingReadings.hum;
    root["readingId"] = String(incomingReadings.readingId);
    String payload;
    serializeJson(root, payload);
    
    Serial.print("event send :");
    serializeJson(root, Serial);
    events.send(payload.c_str(), "new_readings", millis());
    Serial.println();
  }
  if (len == sizeof(pairingData)){                // new code for pairing
    memcpy(&pairingData, incomingData, sizeof(pairingData));
    Serial.print("Pairing request from: ");
    printMAC(mac_addr);
    Serial.println();
    if (pairingData.id > 0) { 
      pairingData.id = 0;  // 0 is server
      // Server is in AP_STA mode: peers need to send data to server soft AP MAC address 
      WiFi.softAPmacAddress(pairingData.macAddr);   
      pairingData.channel = chan;
      Serial.println("send response");
      esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &pairingData, sizeof(pairingData));
    }  
    addPeer(mac_addr); 
  }
}

void initESP_NOW(){
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
} 

void setup() {
  // Initialize Serial Monitor
  Serial.begin(74880);

  Serial.println();
  Serial.print("Server MAC Address:  ");
  Serial.println(WiFi.macAddress());

 

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);


  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }

  Serial.print("Server SOFT AP MAC Address:  ");
  Serial.println(WiFi.softAPmacAddress());

  chan = WiFi.channel();
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  initESP_NOW();
  
  // Start Web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  // Web sockets  
  server.addHandler(&ws);
  
  // Events 
  ws.onEvent(onSocketEvent);
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  
  // start server
  server.begin();

  // Start MDNS (bonjour)
  while(!MDNS.begin("Server")) {
      Serial.println("Starting mDNS...");
      delay(1000);
  }
  Serial.println("MDNS started");
  MDNS.addService("http", "tcp", 80); 

  Serial.println("-----");
  WiFi.printDiag(Serial);
  Serial.println("-----");
}




void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  
    send = 1;
    if (send == 1) {
      outgoingSetpoints.id = 0;
      outgoingSetpoints.temp = 66;
      outgoingSetpoints.hum = 18;
      outgoingSetpoints.readingId = led++;
     
      //esp_now_send(broadcastAddress, (uint8_t *) &outgoingSetpoints, sizeof(outgoingSetpoints));
      esp_now_send(NULL, (uint8_t *) &outgoingSetpoints, sizeof(outgoingSetpoints));
      send = 0;
    }
  }
 }


