#include <SD.h>
#include <arduino.h>
#include "global.h"
#include "Utils.h"
#include "Logger.h"
#include "Config.h"
#include "SPI.h" 
#include "flashLed.h"



struct_LogTemp LT;
std::map<std::string, struct_LogTemp> lt;


unsigned long starting = millis();

const int CS = 17;
const int PWR = 4;

/*
  Caution : micro-sd card needs up to 100mA when writing, do NOT uses 3.3V from esp32 board.
  Uses microSD adapter like Adafruit breakout board 254 and power it from the USB 5V to avoid problems 
  when mounting SD or intermittent readings or writing errrors.
*/
Logger::Logger(){};

void Logger::printTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    safeSerialPrintf("Failed to get time\n");
    return;
  }
  timeinfo.tm_year += 1900;
  safeSerialPrintf("%04d-%02d-%02d %02d:%02d:%02d\n", timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

bool Logger::checkSD(){
   bool status = false;
   if(!xSemaphoreTake( xSemaphore, (400 * portTICK_PERIOD_MS) == pdTRUE)){
      return false;
   } 
   #ifdef SD_DEBUG
      safeSerialPrintf("xSemaphoreTake by checkSD\n"); 
   #endif
  
   if (SD.cardType() != CARD_NONE){
     status = true;
     showDebug(true);
   } else {
      showDebug(true);
    #ifdef SD_DEBUG  
      safeSerialPrintf("SD card is NOT present\n");
    #endif
    initSD();
    if (SDPresent){
      #ifdef SD_DEBUG
        safeSerialPrintf("SD card re-initialized\n");
      #endif
      status = true;
        showDebug(true);

    } else { 
      #ifdef SD_DEBUG 
        safeSerialPrintf("SD card is NOT re-initialized\n");
      #endif
        status = false;
        showDebug(false);

    } 
  }
  xSemaphoreGive(xSemaphore);
  #ifdef SD_DEBUG
    safeSerialPrintf("xSemaphoreGive by checkSD\n");
  #endif
  return status;
}

void Logger::powerOn(uint8_t pin){
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
}

void Logger::powerOff(uint8_t pin){
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}  

void Logger::initSD()
{
  logged = false;
  SDPresent = false;
  pinMode(CS, OUTPUT);
  powerOff(4);
  // reset SD Card by power 
  safeSerialPrintf("SD NOT POWERD\n");
  delay(1000);
  powerOn(4);
  delay(1000);
  safeSerialPrintf("SD POWERD\n");
  //
  SPI.end();
  SPI.begin(18, 19, 23, CS);   // SPI.begin(18, 36, 26, CS); ???
  if (SD.begin(CS)) {
    safeSerialPrintf("SD Mounted\n");
    SDPresent = true;
  } else {
    safeSerialPrintf("An Error has occurred while mounting SD\n");
  }
  #ifndef SERVER_TEST
    if (!SDPresent) { 
    //  showError(13);
    }
  #endif
  #ifdef DEBUG_FS
    safeSerialPrintf("Card Type : ");
    safeSerialPrintf("%d\n", SD.cardType());
    safeSerialPrintf("SD Mounted Total: ");
    safeSerialPrintf("%d\n", SD.totalBytes()/1024);
    safeSerialPrintf(" Used: ");
    safeSerialPrintf("%d\n", SD.usedBytes()/1024);
    safeSerialPrintf(" bytes\n");
  #endif
}

bool Logger::isPresent(){
  return SDPresent; 
}

void  Logger::addToLogData(std::string key, float value)
{
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

bool Logger::createNewFile(String fileName, String Data){
  
  bool result = false;
  File dataFile;

  #ifdef DEBUG_LOGGER
    safeSerialPrintf("Create new data file:  ");
    safeSerialPrintf("%s\n", fileName.c_str());
    safeSerialPrintf("%s\n", Data.c_str());
  #endif
  if (SDPresent) {
    dataFile = SD.open(fileName, "w");
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("SD\n");
    #endif
  }else{
    dataFile = LittleFS.open(fileName, "w");
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("LittleFS\n");
    #endif
  }
  // add JSON structure to new file
  if (dataFile) {
      safeSerialPrintf("%s\n", dataFile.name());
      dataFile.print("{\"Data\":[");
      dataFile.println();
      dataFile.print(Data);
      dataFile.println("]}");
  
      dataFile.flush();
      dataFile.close();
      result = true;
      #ifdef DEBUG_LOGGER
        safeSerialPrintf("Data added to new file\n");
      #endif
  }
  return result;
}  

bool Logger::saveOnSD(String fileName, String Data) {

  bool result = false;
  File dataFile;
  
  #ifdef SERVER_TEST
    bool newfile = !LittleFS.exists(fileName);
  #else   
    bool newfile = !SD.exists(fileName);
  #endif 
  
  if (newfile) {
    createNewFile(fileName, Data);
    return true;
  } 
  #ifdef DEBUG_LOGGER
    safeSerialPrintf("%s\n", Data.c_str());
    safeSerialPrintf("Data file already exists\n");
  #endif
  // do not use "a" or "a+"" because seek don't work in this mode
  if (SDPresent) {
    dataFile = SD.open(fileName, "r+");
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("SD PRESENT\n");
    #endif 
  }else{  
    dataFile = LittleFS.open(fileName, "r+");
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("LittleFS PRESENT\n"); 
    #endif  
  } 
  
  if (dataFile) {
    safeSerialPrintf("%s\n", dataFile.name());
    // erase "]}" at the end of the file
    safeSerialPrintf("Data file is opened\n");
    dataFile.seek(dataFile.size() - 4); 
    // append data
    dataFile.println(",");
    dataFile.println(Data);
           
    // rewrite closing "]}"
    dataFile.print("]}");
    dataFile.flush();
    dataFile.close();
    result = true;
    
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("Data added to existing file\n");
      safeSerialPrintf("%s\n", Data.c_str());
    #endif
  } else {
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("Can not open existing file!\n");
    #endif
  }
  safeSerialPrintf("saveOnSD return TRUE\n");
 // printFile(fileName);
  return result; 
}

void Logger::createFileName(){
// File name is based on Local Time 
  tm timeinfo;
  safeSerialPrintf("before getLocalTime\n");
  getLocalTime(&timeinfo);
  safeSerialPrintf("after getLocalTime\n");

  #ifdef DEBUG_LOGGER
    safeSerialPrintf("Create FileName\n");
  #endif
  sprintf(fileName, "/%04d_%02d_%02d.JSN", 1900+timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  #ifdef DEBUG_LOGGER
    safeSerialPrintf("%s\n", fileName);
  #endif
}


bool Logger::prepareData(){
  safeSerialPrintf("Create FileName\n");
  createFileName();
  tm timeinfo;
  // data are based to UTC
  getUtcTime(&timeinfo);
  #ifdef DEBUG_LOGGER
    //printUtcTime();
  #endif
  sprintf(utc, "%04d-%02d-%02dT%02d:%02d:%02d", 1900+timeinfo.tm_year, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); // UTC time
 // Prepare json
  safeSerialPrintf("CreateJson\n"); 
  StaticJsonDocument<2000> root;
  root["ID"] = "ESP8266";
  root["UID"] = utc;
  char T[10]; 
  char str[40];
  bool hasData = false;
  payload[0] = '\0';
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
  safeSerialPrintf("Json document created\n");
  if (hasData) { // at least one data present
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("Logger has data\n");
    #endif
    serializeJson(root, payload);
  } 
  return (payload[0] != '\0'); 

}

bool Logger::logData(){
  if (saveOnSD(fileName, payload)){
    struct tm LocalTimeinfo;
    getLocalTime(&LocalTimeinfo);
    char utc[25];
    sprintf(utc, "%04d-%02d-%02dT%02d:%02d:%02d", 1900+LocalTimeinfo.tm_year, LocalTimeinfo.tm_mon+1, LocalTimeinfo.tm_mday, LocalTimeinfo.tm_hour,  LocalTimeinfo.tm_min, LocalTimeinfo.tm_sec); // UTC time
    #ifdef DEBUG_LOGGER
      safeSerialPrintf("Data saved on SD\n");
      safeSerialPrintf("Logged at local time  %s\n", utc);
    #endif
    return true; 
  } else {
    return false;
  }
}


void Logger::processLogger() {
  checkSD();
  struct tm LocalTimeinfo;
	getLocalTime(&LocalTimeinfo);
	if (LocalTimeinfo.tm_min % 15 == 0){
    if (!logged){
      //printLocalTime();
      if (prepareData()) {
				if( xSemaphoreTake( xSemaphore, (400 * portTICK_PERIOD_MS) == pdTRUE)){
					safeSerialPrintf("xSemaphoreTake by Logger\n");
					if (logData()) {
						logged = true;
					}
					xSemaphoreGive(xSemaphore);
          safeSerialPrintf("xSemaphoreGive by Logger\n"); 
          
					} else{ 
					safeSerialPrintf("Unable to take xSemaphore\n");
					logged = false;
				} 
			}
			else {
				safeSerialPrintf("No Data available\n");
				logged = false;
			}	
		}  
	} else {
	  logged = false; 
	}  
}  

/*
getLocalTime(&timeinfo);
sprintf(fileName, "/%04d_%02d_%02d.JSN", 1900+timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);

getUtcTime(&timeinfo);
sprintf(utc, "%04d-%02d-%02dT%02d:%02d:%02d", 1900+timeinfo.tm_year, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); // UTC time
 // Prepare json
 



*/

    

