#ifndef LOGGER_H_
#define LOGGER_H_

#include <global.h>
#include <ArduinoJson.h>
#include <map>

class Logger  {
    public:
        Logger();
        void initSD();
        void addToLogData(std::string key, float value);
        void processLogger();
        void printInt(uint8_t data);
    private: 
            //void printTime();
        void saveOnSD(String fileName, String Data);
        bool logDataOnSD();
};    

extern Logger LOG;

#endif
