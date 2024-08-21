#include "Utils.h"
#include <ArduinoJson.h>
#include "global.h"
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Config.h>
#include <print.h>

esp_now_peer_info_t slave;
struct_ping pingData = {};

String peersFilename = "/peersList.js";


void getUtcTime(struct tm * info){
    time_t now;
    time(&now);
    gmtime_r(&now, info);
}

void printUtcTime(){
  Serial.print("UTC Time : ");
  struct tm timeinfo;
  getUtcTime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void printLocalTime(){
  Serial.print("Local Time : ");
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");

}


void checkMem()
{
#ifdef DEBUG_MEM
  Serial.print(F("--- Heap size : "));
  Serial.println(ESP.getFreeHeap());
#endif
}

bool isEqualMAC(const uint8_t *mac_addr1, const uint8_t *mac_addr2)
{
  for (uint8_t i = 0; i < 6; i++)
  {
    if (mac_addr1[i] != mac_addr2[i])
      return false;
  }
  return true;
}

bool addPeerToESPNOW(const uint8_t *peer_addr)
{
  //printlnMAC(peer_addr);
  // Serial.println(PeerID);
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  const esp_now_peer_info_t *peer = &peerInfo;
  esp_err_t getStatus = esp_now_get_peer(peer_addr, &peerInfo);
  if (getStatus == ESP_OK)
  {
    #ifdef DEBUG_PAIRING
        Serial.println("Already Paired");
    #endif
    if (peerInfo.channel != chan)
    {                          // channel has changed
      peerInfo.channel = chan; // set new channel
      peerInfo.encrypt = 0;
      esp_err_t modStatus = esp_now_mod_peer(peer); // modify existing peer
      #ifdef DEBUG_PAIRING
           if (modStatus == ESP_OK)
            {
              Serial.println("Modified success");
            }
            else
            {
              Serial.println("Modified failed");
            }
      #endif
    }
  }
  if (!esp_now_is_peer_exist(peer_addr))
  {
    Serial.println(getStatus);
    peerInfo.channel = chan; // set channel
    peerInfo.encrypt = 0;
    memcpy(peerInfo.peer_addr, peer_addr, 6);     // set MAC address
    esp_err_t addStatus = esp_now_add_peer(peer); // add peer to ESP_NOW
    #ifdef DEBUG_PAIRING
        if (addStatus == ESP_OK)
        {
          Serial.println("Pair success");
        }
        else
        {
          Serial.println("Pair failed");
        }
    #endif
  }
  savePeers("addPeerToESPNOW");
  return true;
}

/*void testPeers(){
    StaticJsonDocument<2000> root;
    root["channel"] = WiFi.channel();
    JsonArray peers = root.createNestedArray("Peers");
    //Serial.println(Peers.size());
    for (int i = 0; i < Peers.size(); i++)
    {
       //if (Peers[i].MAC[0] > 0) {
        JsonObject peer = peers.createNestedObject();
        peer["PeerID"] = Peers[i].PeerID;
        // peer["connected"] = Peers[i].Connected;
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                Peers[i].MAC[0], Peers[i].MAC[1], Peers[i].MAC[2], Peers[i].MAC[3], Peers[i].MAC[4], Peers[i].MAC[5]);
        peer["mac"] = macStr;
        
        char keyName[5];
        for (int j = 0; j < 12; j++)
        {   
          if (Peers[i].deviceTypes[j] < 255) {
            sprintf (keyName, "DevId_%d", j);
            peer[keyName] = Peers[i].deviceTypes[j];
          }  
         // peer["DeviceIds"][j] = Peers[i].deviceIds[j];
         // peer["DeviceTypes"][j] = Peers[i].deviceTypes[j];
        }

    }
    serializeJsonPretty(root,Serial);
}*/

void savePeers(String from)
{
#ifdef DEBUG_SAVE_PEERS
  Serial.print("-- Save peers from ");
  Serial.println(from);
#endif
  File peersFile = LittleFS.open(peersFilename, "w", true);
  if (peersFile) 
  {
    //Serial.println("PeersFile opened");
    StaticJsonDocument<2000> root;
    root["channel"] = WiFi.channel();
    JsonArray peers = root.createNestedArray("Peers");
    //Serial.println(Peers.size());
    for (int i = 0; i < Peers.size(); i++)
    {
       //if (Peers[i].MAC[0] > 0) {
        JsonObject peer = peers.createNestedObject();
        peer["PeerID"] = Peers[i].PeerID;
        // peer["connected"] = Peers[i].Connected;
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                Peers[i].MAC[0], Peers[i].MAC[1], Peers[i].MAC[2], Peers[i].MAC[3], Peers[i].MAC[4], Peers[i].MAC[5]);
        peer["mac"] = macStr;
        for (int j = 0; j < 12; j++)
        {
          peer["DeviceIds"][j] = Peers[i].deviceIds[j];
          peer["DeviceTypes"][j] = Peers[i].deviceTypes[j];
        }
    }

    
    

    #ifdef DEBUG_SAVE_PEERS
      Serial.println("Peer list saved");
      serializeJson(root, Serial);
      Serial.println();
    #endif
    serializeJson(root, peersFile);
    peersFile.close(); 
  } else {
    Serial.println("Unable to create ");
    Serial.println(peersFilename); 
  }
}

void loadPeers() {
  Serial.println(peersFilename);

  if (!LittleFS.exists(peersFilename)) {
    savePeers("loadPeers() create file");
    //#ifdef DEBUG_LOAD_PEERS
        Serial.print(peersFilename);
        Serial.println(" not found, new file created ");
    //#endif 
  } else {
    //#ifdef DEBUG_LOAD_PEERS      
        Serial.println("Loading Peers...");
    //#endif    
    File peersFile = LittleFS.open(peersFilename, "r");
    StaticJsonDocument<1000> root;
    DeserializationError error = deserializeJson(root, peersFile);
    #ifdef DEBUG_LOAD_PEERS    
        serializeJson(root, Serial);
        Serial.println();
        printPeers("From loadPeers()");
    #endif    
    for (int p = 0; p < (int)root["Peers"].size(); p++){  //for each peers in file
      for (int i = 0; i < Peers.size(); i++) { // loop in memory peer list to find PeerID                         
        if (Peers[i].PeerID == root["Peers"][p]["PeerID"]) { 
            
            String macStr = root["Peers"][p]["mac"]; 
            uint8_t mac[6];
            std::sscanf(macStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            
            if (mac[0] > 0) addPeerToESPNOW(mac);  // todo only if mac is not 00 00 00 00 00 00 
            
            memcpy(Peers[i].MAC, mac, 6);
        }
      }
    }
    peersFile.close();
    savePeers("loadPeers()"); 

    #ifdef DEBUG_LOAD_PEERS
        Serial.print(root["Peers"].size());
        Serial.print(" peers updated from ");
        Serial.println(peersFilename);
    #endif
  }
}

bool addPeerToList(const uint8_t *mac_addr, int8_t PeerID)
{
  //#ifdef DEBUG_PAIRING
    printLocalTime();
    Serial.print(" Add peer to list  PeerID : ");
    Serial.println(PeerID);
    printlnMAC(mac_addr);
  //#endif  
  int ndx = -1;
  
  //#ifdef DEBUG_PAIRING
    Serial.print("Before Start Ndx : ");
    Serial.println(ndx);
  //#endif  

  if (!Peers.empty())
  {
    for (int i = 0; i < Peers.size(); i++)
    { // search in list
     // Serial.println(PeerID);
     // Serial.println(Peers[i].PeerID);
      if (Peers[i].PeerID == PeerID) { //|| (isEqualMAC(mac_addr, Peers[i].MAC))) {
		    ndx = i; // PeerID found, save entry index
        Serial.print("PeerID Found, Index= ");
        Serial.println(i);
        break;
	    } 		
    }
  }
  //#ifdef DEBUG_PAIRING
    Serial.print("After search Ndx : ");
    Serial.println(ndx);
  //#endif  
 
  if (ndx < 0) // mac not found in the list
  {
    Peers.push_back(PeerEntry()); // add a new entry
    ndx = Peers.size() - 1;       // set entry index
    Serial.print("PeerID Added, Index = ");
    Serial.println(ndx);
  }
  //#ifdef DEBUG_PAIRING
    Serial.print("Used Ndx : ");
    Serial.println(ndx);
  //#endif  
  Peers[ndx].PeerID = PeerID;
  Peers[ndx].Channel = chan;
  memcpy(Peers[ndx].MAC, mac_addr, 6);
  
  fillPairingData(PeerID);
  
  addPeerToESPNOW(mac_addr);
  #ifdef DEBUG_PAIRING
    printPeers("After Pairing");
  #endif

  //#ifndef SERVER_TEST
    savePeers("addPeerToList()");
    testPeers();
  //#endif 
  
  return true;
}

int macToPeerID(const uint8_t *mac_addr)
{
  int ID = -1;
  for (int i = 0; i < Peers.size(); i++)
  {
    if (isEqualMAC(mac_addr, Peers[i].MAC))
    {
      ID = Peers[i].PeerID;
      break;
    }
  }
  return ID;
}

void SetLastDataReceived(const uint8_t *mac_addr)
{
  uint8_t PeerID = macToPeerID(mac_addr);
  if (PeerID > -1)
  {
   // Peers[PeerID].LastData = millis();
    Serial.print(PeerID);
    Serial.print(" : ");
    Serial.println(millis());
#ifdef DEBUG_PING
    Serial.print("Last Data Received from ");
    printMAC(mac_addr);
    Serial.print(" at ");
    Serial.println("ms");
#endif
  }
  else
  {
#ifdef DEBUG_PING
    Serial.println("Strange Data Received from ");
    printMAC(mac_addr);
#endif
  }
}

uint8_t *PeerIDtoMAC(uint8_t PeerID)
{
  static uint8_t r[6] = {};
  for (int i = 0; i < Peers.size(); i++)
  {
    if (PeerID == Peers[i].PeerID)
    {
      memcpy(r, Peers[i].MAC, 6);
      return r;
      break;
    }
  }
  return r;
}

/*File OpenFile(const char *fileName, const char *mode = "r")
{
  File f = {};
  File root = LittleFS.open("/");
  f = root.openNextFile(mode);
  while (f)
  {
    // Serial.println(f.name());
    if (strcmp(f.name(), fileName) == 0)
    {
      // Serial.println("found");
      return f;
    }
    f.close();
    f = root.openNextFile();
  }
  return f;
}*/

void print_m()
{
  Serial.println("Data to log");
  for (m_it = m.begin(); m_it != m.end(); m_it++) {
      if (m_it->second.c_str() > "") {
        Serial.print(m_it-> first.c_str());
        Serial.print(" = ");
        Serial.println(m_it->second.c_str());
      }
  }
  Serial.println("End of Data to log");
}

//  Create a list of all devices from congiguration file
bool fillDevices(String fileName)
{
	uint8_t PID;
	uint8_t DID;
	
	Serial.print("Fill devices from ");
	Serial.println(fileName);
	StaticJsonDocument<2048> doc;
	File dataFile = LittleFS.open(fileName);
  if (dataFile){
		Serial.println(" Config file Opened");
		deserializeJson(doc, dataFile);
		#ifdef DEBUG_DATA_STRUCTURE
      Serial.println("loading DataStructure");
    #endif  
		JsonArray P = doc["Struct"][0]["Peers"];
		JsonArray DT = doc["Struct"][1]["DataTypes"];
		char keyStr[30];
		//serializeJsonPretty(P,Serial); 
    //Serial.println();
    for (int i = 0; i < P.size(); i++) {      	  // loop on Peers
	    Peers.push_back(PeerEntry());
			JsonArray D = P[i]["Devices"];
      Peers[i].PeerID = P[i]["PeerID"];
      for (int j = 0; j < D.size(); j++){		  // loop on Devices
				Peers[i].deviceIds[j] = D[j]["ID"].as<int>();
				Peers[i].deviceTypes[j] = D[j]["Type"].as<int>();
				Peers[i].controlIds[j] = D[j]["CtrlID"].as<int>();
        
        for (int m = 0; m < 6; m++){
          Peers[i].MAC[m] = 0;
        }

        PID = Peers[i].PeerID;
				DID = D[j]["ID"];
        
        JsonObject Cells = DT[D[j]["Type"]];     
        for (JsonPair kv : Cells) {
					snprintf(keyStr, sizeof(keyStr), "%s_%d_%d",kv.key().c_str(),PID,DID);
          String k = kv.value();
					m.insert(std::pair<std::string, std::string>((std::string)keyStr, (std::string)kv.value().as<const char*>()));
				}
      }
      for (int j = D.size(); j < 12; j++){
        Peers[i].deviceIds[j] = 255;
        Peers[i].deviceTypes[j] = 255;
        Peers[i].controlIds[j] = 0;
      }
		}
    //print_m();
    
    //printPeers("After fillDevices");
	  
    dataFile.close();
    
    return true;
  } else {
    Serial.print("Error reading ");
    Serial.println(fileName);
    return false;
  } 
    
}

uint8_t getDeviceType(uint8_t PeerID, uint8_t DeviceID)
{
  //Serial.println("getDeviceType");
  for (int i = 0; i < Peers.size(); i++)
  {
    if (Peers[i].PeerID == PeerID)
    {
      for (int j = 0; j < 12; j++)
      {
        if (Peers[i].deviceIds[j] == DeviceID)
        {
          return Peers[i].deviceTypes[j];
        }
      }
    }
  }
  return 255;
}

void fillPairingData(uint8_t PeerID)
{ 
  Serial.print("Fill pairing data for PeerID ");
  Serial.print(PeerID);
  Serial.println();
  
  for (int i = 0; i < Peers.size(); i++)
  {
    if (Peers[i].PeerID == PeerID)
    {  
      for (int j = 0; j < 12; j++)
      {
        // fill pairingData
        pairingData.deviceTypes[j] = Peers[i].deviceTypes[j];
        pairingData.deviceIds[j] = Peers[i].deviceIds[j];
        pairingData.controlNdx[j] = Peers[i].controlIds[j];
      }
    }
  }
  #ifdef DEBUG_PAIRING
    printPairingData();
  #endif  
}

void showDirectory()
{
  #ifdef SERVER_TEST
    File root = LittleFS.open("/");
    Serial.println("LittleFS Card Directory");
  #else
    File root = SD.open("/");
    Serial.println("SD Card Directory");
  #endif  
  File f = root.openNextFile();
  while (f)
  {
    Serial.print("FILE: ");
    Serial.print(f.name());
    Serial.print("  S=");
    Serial.println(f.size());

    f = root.openNextFile();
  }
  root.close();
}

void copyLittleFStoSD()
{
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f)
  {
    Serial.print("FILE: ");
    Serial.print(f.name());
    Serial.print("  S=");
    Serial.println(f.size());
    String f2Name = "/";
    f2Name += f.name();
    File f2 = SD.open(f2Name,"w");
    size_t n;  
    uint8_t buf[128];
    while ((n = f.read(buf, sizeof(buf))) > 0) {
      f2.write(buf, n);
    }
    f2.close(); // done, close the destination file
    f = root.openNextFile();
  }
  root.close();
}



