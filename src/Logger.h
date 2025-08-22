#ifndef LOGGER_H_
#define LOGGER_H_

#include <global.h>
#include <ArduinoJson.h>
#include <map>
#include "Utils.h"
#include "global.h"
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Config.h>
#include <print.h>

class Logger  {
    public:
        Logger();
        void powerOn(uint8_t pin);
        void powerOff(uint8_t pin);
        void initSD();
        void addToLogData(std::string key, float value);
        void processLogger();
        bool logData();
        bool isPresent();
        void printTime();
        void clearSD();
        bool saveOnSD(String fileName, String Data);
        bool saveOnSD_old(String fileName, String Data);
        bool checkSD();
    private: 
        void createFileName();
        bool createNewFile(String fileName, String Data);
        bool prepareData();
        bool logDataOnSD();
        bool dataPrepared;
        bool logged;
        bool SDPresent;
        char fileName[25];
        char utc[25];
        char payload[2000];
};    

extern Logger LOG;

#endif
