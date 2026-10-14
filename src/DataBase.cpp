#include "Utils.h"
#include <ArduinoJson.h>
#include "global.h"
#include <LittleFS.h>
#include <esp_now.h>
#include <print.h>
#include <DataBase.h>
#include "Config.h"
#include <Preferences.h>


DataBase::DataBase(){};



bool DataBase::addSetPoint(uint8_t PeerID, uint8_t DeviceID, uint8_t value){
  char Key[10];
  sprintf(Key, "%02d-%02d", PeerID, DeviceID);
  SetPointEntry SP = {};
  SP.PeerID = PeerID;
  SP.DeviceID = DeviceID;
  SP.SetPoint = value;
  setpointsList.putBytes(Key, &SP, sizeof(SP));
  //printSetPoints();
  return true;
}

void DataBase::printSetPoints(){
  char Key[10]; 
  SetPointEntry SP = {}; 
  safeSerialPrintf("List of setPoints \n"); 
  for (int i = 0; i < DB.Peers.size(); i++) {
    for (int j = 0; j < 12; j++){
      sprintf(Key, "%02d-%02d", DB.Peers[i].PeerID, DB.Peers[i].deviceIds[j]);
      if (setpointsList.isKey(Key)){
        setpointsList.getBytes(Key, &SP, sizeof(SP));
        safeSerialPrintf("%s = %d\n", &Key, SP.SetPoint);
      }
    }    
  }
  safeSerialPrintf("End of setPoints \n");  
} 

void DataBase::sendAllSetPoints(){
  struct_message Setpoint;
  char Key[10]; 
  SetPointEntry SP = {}; 
  for (int i = 0; i < DB.Peers.size(); i++) {
    for (int j = 0; j < 12; j++){
      sprintf(Key, "%02d-%02d", DB.Peers[i].PeerID, DB.Peers[i].deviceIds[j]);
      if (setpointsList.isKey(Key)){
        setpointsList.getBytes(Key, &SP, sizeof(SP));
        safeSerialPrintf("%s = %d sended\n", &Key, SP.SetPoint);
       
       
        Setpoint.msgType = SETPOINTS;
        Setpoint.deviceId = SP.DeviceID;
        Setpoint.deviceType = DB.getDeviceType(SP.PeerID, SP.DeviceID);
        Setpoint.U1 = SP.SetPoint;
        esp_err_t result = esp_now_send(DB.Peers[i].MAC, (uint8_t *)&Setpoint, sizeof(Setpoint));
        if (result == ESP_OK) {
          safeSerialPrintf("Setpoint sent successfully\n");
        } else {
          safeSerialPrintf("Error sending setpoint\n");
        }
      }
    }    
  }
}



bool DataBase::init(String structFileName, String peersFilename){
  if (fillDevices(structFileName)) {
    return DataBase::loadPeers(peersFilename);
  }else{
    return false;
  }
}

bool DataBase::isEqualMAC(const uint8_t *mac_addr1, const uint8_t *mac_addr2) // udes by addPeerToList
{
  for (uint8_t i = 0; i < 6; i++)
  {
    if (mac_addr1[i] != mac_addr2[i])
      return false;
  }
  return true;
}


int DataBase::macToPeerID(const uint8_t *mac_addr){
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

uint8_t* DataBase::PeerIDtoMAC(uint8_t PeerID)
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

bool DataBase::addPeerToESPNOW(const uint8_t *peer_addr, uint8_t peerID){
 // safePrintlnMAC(peer_addr);
  //safeSerialPrintf(PeerID);
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  const esp_now_peer_info_t *peer = &peerInfo;
 
    //safeSerialPrintf(getStatus);
    esp_now_del_peer(peer_addr); // delete peer if already exists
    peerInfo.channel = 0; // set channel
    peerInfo.encrypt = 0;
    memcpy(peerInfo.peer_addr, peer_addr, 6);     // set MAC address
    esp_err_t addStatus = esp_now_add_peer(peer); // add peer to ESP_NOW
    #ifdef DEBUG_PAIRING
        if (addStatus == ESP_OK)
        {
          safeSerialPrintf("Pair success for PeerId %d, MAC ", peerID);
          printMAC(peer_addr);
          safeSerialPrintf("\n");
        }
        else
        {
          safeSerialPrintf("Pair failed\n");
        }
    #endif
  //}
  //savePeers("addPeerToESPNOW");
  return true;
}

bool DataBase::addPeerToList(const uint8_t *mac_addr, int8_t PeerID){
  #ifdef DEBUG_PAIRING
    safeSerialPrintf("Add peer to list for PeerID %d \n", PeerID);
    safeSerialPrintf("MAC Address: ");
    printMAC(mac_addr);
    safeSerialPrintf("\n");
  #endif  
  int ndx = -1;
  
  //#ifdef DEBUG_PAIRING
   // safeSerialPrintf("Before Start Ndx : ");
   // safeSerialPrintf(ndx);
  //#endif

  if (!Peers.empty())
  {
    for (int i = 0; i < Peers.size(); i++)
    { // search in list
     // safeSerialPrintf(PeerID);
     // safeSerialPrintf(Peers[i].PeerID);
      if (Peers[i].PeerID == PeerID) { //|| (isEqualMAC(mac_addr, Peers[i].MAC))) {
		    ndx = i; // PeerID found, save entry index
     //   safeSerialPrintf("PeerID Found, Index= ");
     //   safeSerialPrintf(i);
        break;
	    } 		
    }
  }
  //#ifdef DEBUG_PAIRING
    //safeSerialPrintf("After search Ndx : ");
    //safeSerialPrintf(ndx);
  //#endif

  if (ndx < 0) // mac not found in the list
  {
    Peers.push_back(PeerEntry()); // add a new entry
    ndx = Peers.size() - 1;       // set entry index
   // safeSerialPrintf("PeerID Added, Index = ");
   // safeSerialPrintf(ndx);
  }
  //#ifdef DEBUG_PAIRING
  //  safeSerialPrintf("Used Ndx : ");
  //  safeSerialPrintf(ndx);
  //#endif  
  Peers[ndx].PeerID = PeerID;
  Peers[ndx].Channel = chan;
  memcpy(Peers[ndx].MAC, mac_addr, 6);
  
  fillPairingData(PeerID);
  
  addPeerToESPNOW(mac_addr, PeerID );
  //#ifndef SERVER_TEST
    savePeers("addPeerToList()");
  //#endif 
  
  return true;
}

void DataBase::savePeers(String from){
#ifdef DEBUG_SAVE_PEERS
    safeSerialPrintf("-- Save peers from ");
  safeSerialPrintf(from);
#endif
  File peersFile = LittleFS.open(peersFilename, "w", true);
  if (peersFile) 
  {
    //safeSerialPrintf("PeersFile opened");
    StaticJsonDocument<2000> root;
    root["channel"] = WiFi.channel();
    JsonArray peers = root.createNestedArray("Peers");
    //safeSerialPrintf(Peers.size());
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
      safeSerialPrintf("Peer list saved");
      serializeJson(root, Serial);
      safeSerialPrintf();
    #endif
    serializeJson(root, peersFile);
    //Serial.println("From savePeers");
    //serializeJsonPretty(root, Serial);
    peersFile.close(); 
  } else {
    safeSerialPrintf("Unable to create ");
    safeSerialPrintf(peersFilename.c_str());
  }
}

void  DataBase::clearPeers(){
  LittleFS.remove(peersFilename);
  
}

bool DataBase::loadPeers(String peersFilename) {
  //safeSerialPrintf(peersFilename);

  if (!LittleFS.exists(peersFilename)) {
    savePeers("loadPeers() create file");
    //#ifdef DEBUG_LOAD_PEERS
        safeSerialPrintf(peersFilename.c_str());
        safeSerialPrintf(" not found, new file created ");
    //#endif 
  } else {
    safeSerialPrintf("Loading Peers from ");
    safeSerialPrintf("%s\n",peersFilename.c_str());
        
    File peersFile = LittleFS.open(peersFilename, "r");
    StaticJsonDocument<2000> root;
    DeserializationError error = deserializeJson(root, peersFile);
    #ifdef DEBUG_LOAD_PEERS    
        serializeJson(root, Serial);
        safeSerialPrintf.println();
        printPeers("From loadPeers()");
    #endif    

//    safeSerialPrintf((int)root["Peers"].size());
//    safeSerialPrintf(Peers.size());



    for (int p = 0; p < (int)root["Peers"].size(); p++){  //for each peers in file
       for (int i = 0; i < Peers.size(); i++) { // loop in memory peer list to find PeerID                       
        if (Peers[i].PeerID == root["Peers"][p]["PeerID"]) { 
            
            String macStr = root["Peers"][p]["mac"]; 
            uint8_t mac[6];
            std::sscanf(macStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            
            addPeerToESPNOW(mac, Peers[i].PeerID);   
            memcpy(Peers[i].MAC, mac, 6);
        }
      }
    }
    peersFile.close();
    savePeers("loadPeers()"); 

    #ifdef DEBUG_LOAD_PEERS
        safeSerialPrintf(root["Peers"].size());
        safeSerialPrintf(" peers updated from ");
        safeSerialPrintf(peersFilename);
    #endif
  }
  return true;
}


bool DataBase::fillDevices(String fileName)
{
	uint8_t PID;
	uint8_t DID;

	safeSerialPrintf("Fill devices from ");
	safeSerialPrintf("%s\n", fileName.c_str());
	StaticJsonDocument<2048> doc;
	File dataFile = LittleFS.open(fileName);
  if (dataFile){
		safeSerialPrintf("Config file Opened\n");
		deserializeJson(doc, dataFile);
		#ifdef DEBUG_DATA_STRUCTURE
      safeSerialPrintf("loading DataStructure");
    #endif
		JsonArray P = doc["Peers"];
		JsonArray DT = doc["DataTypes"];
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
        

        /* create a dictionnary of HTML elements ID's suffix <-> LogType */
        JsonObject Cells = DT[D[j]["Type"]];     
        for (JsonPair kv : Cells) {
					snprintf(keyStr, sizeof(keyStr), "%s_%d_%d",kv.key().c_str(),PID,DID);
          SuffixToLogType_Map.insert(std::pair<std::string, std::string>((std::string)keyStr, (std::string)kv.value().as<const char*>()));
				}
      }

      for (int j = D.size(); j < 12; j++){
        Peers[i].deviceIds[j] = 255;
        Peers[i].deviceTypes[j] = 255;
        Peers[i].controlIds[j] = 0;
      }
		}
    dataFile.close();
    safeSerialPrintf("Done");
    return true;
  } else {
    safeSerialPrintf("Error reading ");
    safeSerialPrintf(fileName.c_str());
    return false;
  } 
    
}

void DataBase::testPeers(){
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
        
        char keyName[32];
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
    safeSerialPrintf("From TestPeers");
    serializeJsonPretty(root,Serial);
}

uint8_t DataBase::getDeviceType(uint8_t PeerID, uint8_t DeviceID){
  for (int i = 0; i < Peers.size(); i++){
    if (Peers[i].PeerID == PeerID)
    {for (int j = 0; j < 12; j++)
      {
        if (Peers[i].deviceIds[j] == DeviceID){
          return Peers[i].deviceTypes[j];
        }
      }
    }
  }
  return 255;
}

void DataBase::fillPairingData(uint8_t PeerID)
{ 
  #ifdef DEBUG_PAIRING
    safeSerialPrintf("Fill pairing data for PeerID %d\n", PeerID);
  #endif

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
    //printPairingData();
  #endif  
}

void DataBase::printPeersFromFile() {
    File peersFile = LittleFS.open(peersFilename, "r");
    if (peersFile) {
      StaticJsonDocument<2000> root;
      DeserializationError error = deserializeJson(root, peersFile);
      serializeJsonPretty(root, Serial);
      safeSerialPrintf("\n");
      peersFile.close();
    } else {
      safeSerialPrintf("Unable to open ");
      safeSerialPrintf(peersFilename.c_str());
    }
  
}  