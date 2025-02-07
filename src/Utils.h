#ifndef UTILS_H_
#define UTILS_H_

#include <arduino.h>
#include <global.h>
#include <print.h>



//uint8_t* deviceAddressToString (const uint8_t *device_addr);
/*void printDeviceAddress(const uint8_t *device_addr);
void printMAC(const uint8_t *mac_addr);
void printlnMAC(const uint8_t *mac_addr);*/
bool isEqualMAC(const uint8_t *mac_addr1, const uint8_t *mac_addr2);
void setChannelAndPeerID(const uint8_t *mac_addr, uint8_t channel, uint8_t PeerID);
int macToPeerID(const uint8_t *mac_addr);
uint8_t* PeerIDtoMAC(uint8_t ndx);
bool addPeerToESPNOW(const uint8_t *peer_addr);

void SetLastDataReceived(const uint8_t *mac_addr);

bool fillDevices(String fileName);
    /* Read Structure  JSON to 
        - Create a memory list of Peers with devices information
            PeerID, Type, MAC, Channel, pairing data (deviceIds, deviceTypes, controlIds)
            MAC is unknow at this moment. 
        - Build dictionnaries for linking data to html elements    
    */

void savePeers(String from);
    /* Save list of peers to LittleFS
    */

void loadPeers();
    /*  
        Create a new file if they don't exists  
        Load Peers list from LittleFS
        Create Peers list in memory
        Add all MAC addresses and WiFi Channel to ESP_NOW peer list
    */

bool addPeerToList( const uint8_t *mac_addr, int8_t PeerID);
    /*  
        Called by the pairing process 
        Check if the PeerId is present in the peers list 
        **** TODO Display a warning to the user in case of new PeerId ****  
        Update the MAC adress
        Update pairing data 
    */

void fillPairingData(uint8_t PeerID);
uint8_t getDeviceType(uint8_t PeerID, uint8_t DeviceID);

void getUtcTime(struct tm * info);
void printUtcTime();
void printLocalTime();

void copyLittleFStoSD();


//void printPeers();


        
void showDirectory();
void copyLittleFStoSD();

void checkMem();

void print_m();

void ReadRTCdata();


#endif