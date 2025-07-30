#include <print.h>

void printIncomingData(){
    Serial.print("msgType : ");
    Serial.println(incomingReadings.msgType);
    Serial.print("ID : ");
    Serial.println(incomingReadings.deviceId);
    Serial.print("DeviceType : ");
    Serial.println(incomingReadings.deviceType);
    Serial.print("F1 : ");
    Serial.println(incomingReadings.F1);
    Serial.print("F2 : ");
    Serial.println(incomingReadings.F2);
    Serial.print("U1 : ");
    Serial.println(incomingReadings.U1);
    Serial.print("U2 : ");
    Serial.println(incomingReadings.U1);
    Serial.println("-------------------");
}

void printPairingData(){
  Serial.print("msgType : "); 
  Serial.println(pairingData.msgType);
  Serial.print("netWork : "); 
  Serial.println(pairingData.network);
  Serial.print("macAddr : "); 
  for(int i = 0; i < 6; i++){
      Serial.print(pairingData.macAddr[i]);
      Serial.print(", ");
  } 
  Serial.println();
 
  Serial.print("Channel : ");
  Serial.println(pairingData.channel);
   
  Serial.print("deviceId : ");
  for(int i = 0; i < 12; i++){
      Serial.print(pairingData.deviceIds[i]);
      Serial.print(", ");
  } 
  Serial.println(); 

  Serial.print("deviceTypes : ");  
  for(int i = 0; i < 12; i++){
      Serial.print(pairingData.deviceTypes[i]);
      Serial.print(", ");
  } 
  Serial.println();
 
  Serial.print("controlNdx : ");
  for(int i = 0; i < 12; i++){
      Serial.print(pairingData.controlNdx[i]);
      Serial.print(", ");
  } 
  Serial.println();
  
  Serial.println("");
}

void printDeviceAddress(const uint8_t *device_addr)
{
  char deviceStr[30];
  snprintf(deviceStr, sizeof(deviceStr), "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
           device_addr[0], device_addr[1],device_addr[2], device_addr[3], device_addr[4], device_addr[5], device_addr[6], device_addr[7]);
  Serial.println(deviceStr);
}

void printMAC(const uint8_t *mac_addr)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

void printlnMAC(const uint8_t *mac_addr)
{
  printMAC(mac_addr);
  Serial.println();
}

void printPeer(uint8_t ndx, bool full = false)
{
  Serial.println(ndx);
  Serial.print("     PeerID :");
  Serial.println(Peers[ndx].PeerID);
  Serial.print("        MAC : ");
  printlnMAC(Peers[ndx].MAC);
  Serial.print("    Channel :");
  Serial.println(Peers[ndx].Channel);
  if (full){
    Serial.print("  DeviceIDs : ");
    for (int j = 0; j < 12; j++)
    {
      Serial.print(Peers[ndx].deviceIds[j]);
      Serial.print(", ");
    }
    Serial.println();
    Serial.print("DeviceTypes : ");
    for (int j = 0; j < 12; j++)
    {
      Serial.print(Peers[ndx].deviceTypes[j]);
      Serial.print(", ");
    }
     Serial.println();
    Serial.print("ControlNdx : ");
    for (int j = 0; j < 12; j++)
    {
      Serial.print(Peers[ndx].controlIds[j]);
      Serial.print(", ");
    }
  }
  Serial.println();
  Serial.println("-------------");
}

void printPeers(String raison)
{
  Serial.print("==== Peers List ==== ");
  Serial.println(raison);
  for (int i = 0; i < Peers.size(); i++)
  {
    printPeer(i, false);
  }
  Serial.println("==== End of Peers List ====");
}