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
        void initSD();
        void addToLogData(std::string key, float value);
        void processLogger();
    private: 
        void saveOnSD(String fileName, String Data);
        bool logDataOnSD();
};    

extern Logger LOG;

#endif
