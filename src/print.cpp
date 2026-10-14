#include <print.h>
#include <Utils.h>

void printIncomingData(){
    safeSerialPrintf("msgType : %d\n ", incomingReadings.msgType);
    safeSerialPrintf("ID : %d\n",incomingReadings.deviceId);
    safeSerialPrintf("DeviceType : %d\n",incomingReadings.deviceType );
    safeSerialPrintf("F1 : %d\n", incomingReadings.F1);
    safeSerialPrintf("F2 : %d\n", incomingReadings.F2);
    safeSerialPrintf("U1 : %d\n", incomingReadings.U1);
    safeSerialPrintf("U2 : %d\n", incomingReadings.U1);
    safeSerialPrintf("-------------------\n");
}

void printPairingData(){
  safeSerialPrintf("msgType : %d\n", pairingData.msgType); 
  safeSerialPrintf("netWork : %d\n", pairingData.network); 
  safeSerialPrintf("macAddr : "); 
  for(int i = 0; i < 6; i++){
    safeSerialPrintf("%02X:", pairingData.macAddr[i]);  
  } 
  safeSerialPrintf("\n");
  safeSerialPrintf("Channel : %d\n",pairingData.channel);
  safeSerialPrintf("deviceId : ");
  for(int i = 0; i < 12; i++){
      safeSerialPrintf("%d ,",pairingData.deviceIds[i]);
  } 
  safeSerialPrintf("\n"); 

  safeSerialPrintf("deviceTypes : ");  
  for(int i = 0; i < 12; i++){
      safeSerialPrintf("%d ,",pairingData.deviceTypes[i]);
  } 
  safeSerialPrintf("\n");
 
  safeSerialPrintf("controlNdx : ");
  for(int i = 0; i < 12; i++){
      safeSerialPrintf("%d ,",pairingData.controlNdx[i]);
  } 
  safeSerialPrintf("\n\n");
}

void printDeviceAddress(const uint8_t *device_addr)
{
  char deviceStr[30];
  snprintf(deviceStr, sizeof(deviceStr), "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
           device_addr[0], device_addr[1],device_addr[2], device_addr[3], device_addr[4], device_addr[5], device_addr[6], device_addr[7]);
  safeSerialPrintf(deviceStr);
}

String macToString(const uint8_t *mac_addr){
  char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  return macStr;         
}

void printMAC(const uint8_t *mac_addr)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  safeSerialPrintf("%s", macStr);
}

/*void printlnMAC(const uint8_t *mac_addr)
{
   safeSerialPrintf("%s\n",mac_addr);
}*/

void printPeer(uint8_t ndx, bool full = false)
{
  safeSerialPrintf("",ndx);
  safeSerialPrintf("     PeerID :%d\n",Peers[ndx].PeerID);
  safeSerialPrintf("        MAC :%d\n ",Peers[ndx].MAC);
  safeSerialPrintf("    Channel :%d\n",Peers[ndx].Channel);

  if (full){
    safeSerialPrintf("  DeviceIDs : ");
    for (int j = 0; j < 12; j++)
    {
      safeSerialPrintf(", %d",Peers[ndx].deviceIds[j]);
    }
    safeSerialPrintf("\n");
    
    safeSerialPrintf("DeviceTypes : ");
    for (int j = 0; j < 12; j++)
    {
      safeSerialPrintf(", %d",Peers[ndx].deviceTypes[j]);
    }
    safeSerialPrintf("\n");
    
    safeSerialPrintf("ControlNdx : ");
    for (int j = 0; j < 12; j++)
    {
      safeSerialPrintf(", %d",Peers[ndx].controlIds[j]);
    }
    safeSerialPrintf("\n");
  }
  safeSerialPrintf("-------------");
}

void printPeers(String raison)
{
  safeSerialPrintf("==== Peers List ====  from %s ", raison);
  
  for (int i = 0; i < Peers.size(); i++)
  {
    printPeer(i, false);
  }
  safeSerialPrintf("==== End of Peers List ====\n");
}