#include <SD.h>
#include <arduino.h>
#include "global.h"
#include "Utils.h"
#include "Logger.h"
#include "Config.h"
#include "SPI.h" 

struct_LogTemp LT;
std::map<std::string, struct_LogTemp> lt;

unsigned long starting = millis();

const int CS = 5;


bool logged = true;

void printTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to get time");
    return;
  }
  Serial.println(&timeinfo, "%y-%m-%d %H:%M:%S");
}

void clearSD(){
  byte sd = 0;
  digitalWrite(CS, LOW);
  while (sd != 255){
    sd = SPI.transfer(255);
    Serial.print("sd = ");
    Serial.println(sd);
    delay(100);
  }
  digitalWrite(CS, HIGH);
}

/*
  Caution : micro-sd card needs up tp 100mA when writing, do NOT uses 3.3V from esp32 board.
  Uses microSD adapter like Adafruit breakout board 254 and power it from the USB 5V to avoid problems 
  when mounting SD or intermittent readings or writing errrors.
*/
void initSD()
{
  pinMode(CS, OUTPUT);
  SPI.end();
  SPI.begin(18, 19, 23, CS); 
  
  SPI.setDataMode(SPI_MODE0); 
  Serial.println(SCK);
  Serial.println(MISO);
  Serial.println(MOSI);
  Serial.println(CS);
  boolean b;
  printTime();
  if (SD.begin(CS)) {
    Serial.println("SD Mounted");
  } else {
    clearSD();
    b = SD.begin(CS); 
    if (b) {
      Serial.println("SD Mounted after clearing SD");
    } else {  
      Serial.println("An Error has occurred while mounting SD");
    }  
  }
}

void addToLogData(std::string key, float value)
{
  //Serial.println(key.c_str());
  auto a = lt.find(key);
  if(a!= lt.end()){
    a->second.F1 += value;
    a->second.N++;
  }else{
    LT.F1 = value;
    LT.N = 1;
    lt.insert(std::pair<std::string, struct_LogTemp>(key, LT));
  } 
};

void saveOnSD(String fileName, String Data) {
  bool newfile = !SD.exists(fileName);
  if (newfile) {
     Serial.print("Create new data file:  ");
     Serial.println(fileName);
     File dataFile = SD.open(fileName, "w", true); 
     
    // add JSON structure to new file
    if (dataFile) {
        dataFile.print("{\"Data\":[");
        dataFile.print(Data);
        dataFile.print("]}");
        dataFile.flush();
        dataFile.close();
        Serial.println("Data added to new file"); 
    }
  }
  else
  {
    Serial.println("Data file already exists");
    // do not use "a" or "a+"" because seek don't work in these modes
    File dataFile = SD.open(fileName, "r+");
    if (dataFile) {
      // erase "]}" at the end of the file
      Serial.println("Data file is opened ");
      dataFile.seek(dataFile.size() - 2);
      // append separator
      dataFile.print(",");
      // append data
      dataFile.print(Data);
      // rewrite closing "]}"
      dataFile.print("]}");
      dataFile.flush();
      dataFile.close();
      Serial.println("Data added to existing file");
    } else {
      Serial.println("Can not open existing file!");
    }
  }  
}

bool logDataOnSD()
{   
  tm timeinfo;
  // File name is based on Local Time  
  char fileName[25];
  getLocalTime(&timeinfo);
  #ifdef DEBUG_LOGGER
    printLocalTime();
  #endif
  sprintf(fileName, "/%02d_%02d.JSN", timeinfo.tm_mon + 1, timeinfo.tm_mday);
  #ifdef DEBUG_LOGGER
    Serial.println(fileName); 
  #endif
  // data are based to UTC
  getUtcTime(&timeinfo);
  #ifdef DEBUG_LOGGER
    printUtcTime();
  #endif
  char utc[25];
  sprintf(utc, "%02d-%02d-%02dT%02d:%02d:%02d", 2023, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); // UTC time
 

  // Prepare json
  StaticJsonDocument<2000> root;
  root["ID"] = "ESP8266";
  root["UID"] = utc;
  char T[10]; 
  char str[40];
  bool hasData = false;
  // fill data
  for (it = lt.begin(); it != lt.end(); it++) {  //fill data from all logs
    
    if (it->second.N > 0){
      hasData = true;
      sprintf(str, "%.2f", it->second.F1 / it->second.N);
      root[it->first.c_str()] = str;
      // reset data in logger 
      it->second.F1 = 0;
      it->second.N = 0;
    }  
  }
  // 
  if (hasData) { // at least one data present
    char payload[2000];
    #ifdef DEBUG_LOGGER
      serializeJson(root, Serial);
    #endif  
    serializeJson(root, payload);
    //initSD();
    saveOnSD(fileName, payload);
  }
  return true;
}

void processLogger()
{
    struct tm LocalTimeinfo;
    getLocalTime(&LocalTimeinfo);
    if (LocalTimeinfo.tm_min % 15 == 0){
      if (!logged){
        logged = true;
        canHandle = false;
        logDataOnSD();
        
        canHandle = true;
        char utc[25];
        sprintf(utc, "%02d-%02d-%02dT%02d:%02d:%02d", 2023, LocalTimeinfo.tm_mon+1, LocalTimeinfo.tm_mday, LocalTimeinfo.tm_hour,  LocalTimeinfo.tm_min, LocalTimeinfo.tm_sec); // UTC time
        Serial.print("Logged at local time  ");
        Serial.println(utc);
      }  
    } else {
        logged = false; 
    }
}

    

