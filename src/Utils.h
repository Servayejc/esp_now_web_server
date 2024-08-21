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
bool addPeerToList( const uint8_t *mac_addr, int8_t PeerID);
void SetLastDataReceived(const uint8_t *mac_addr);
bool fillDevices(String fileName);
void fillPairingData(uint8_t PeerID);
uint8_t getDeviceType(uint8_t PeerID, uint8_t DeviceID);

void getUtcTime(struct tm * info);
void printUtcTime();
void printLocalTime();

void copyLittleFStoSD();


//void printPeers();
void savePeers(String from);
void loadPeers();
void loadDataStructure(uint8_t PeerId);
void showDirectory();
void copyLittleFStoSD();

void checkMem();

void print_m();

#endif