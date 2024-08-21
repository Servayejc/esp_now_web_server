#ifndef PRINT_H_
#define PRINT_H_

#include <Utils.h>
#include <ArduinoJson.h>
#include <global.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

void printPairingData();
void printIncomingData();
void printDeviceAddress(const uint8_t *device_addr);
void printMAC(const uint8_t *mac_addr);
void printlnMAC(const uint8_t *mac_addr);
void printPeers(String raison);

#endif