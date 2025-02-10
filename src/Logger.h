#ifndef LOGGER_H_
#define LOGGER_H_

#include <global.h>
#include <ArduinoJson.h>
#include <map>

void printTime();
void initSD();
void addToLogData(std::string key, float value);
void processLogger();

void saveOnSD(String fileName, String Data);
//bool logDataOnSD(tm timeinfo);


#endif
