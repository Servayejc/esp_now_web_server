#ifndef SVR_H_
#define SVR_H_

#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <SPIFFS.h>

void startServer();
void initESP_NOW();
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
/*
    - read first byte of the message (this is the messageType) 
    - if messageType is DATA transfer to ProcessDataReceived function
    - if messageType is PAIRING
        - copy esp_now message to pairingData structure
        - verify if pairingData.network == SERVER_ID
        - add the new peer to Peers structure via addPeerToList(const uint8_t *mac_addr, int8_t PeerID)
*/
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);


void setActivePeers();


/*
void ProcessDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
    - copy esp_now message to incomingData structure
    - transform data to JSON object according to device type
    - send JSON to logger
    - send JSON to web clients via events   

void sendSetPoints(int8_t peerId);
    - send new set point to corresponding peer and device

*/

#endif