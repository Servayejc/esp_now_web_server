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
    Serial.println("Failed to get time");
    return;
  }
  Serial.println(&timeinfo, "%y-%m-%d %H:%M:%S");
}

void Logger::clearSD(){
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



void Logger::initSD()
{
  logged = false;
  SDPresent = false;
  pinMode(CS, OUTPUT);
  SPI.end();
  SPI.begin(18, 19, 23, CS);   // SPI.begin(18, 36, 26, CS); ???
  if (SD.begin(CS)) {
    Serial.println("SD Mounted");
    SDPresent = true;
  } else {
    Serial.println("An Error has occurred while mounting SD"); 
  }
  #ifndef SERVER_TEST
    if (!SDPresent) { 
      showError(13);
    }
  #endif
  #ifdef DEBUG_FS
    Serial.print("Card Type : ");
    Serial.println(SD.cardType());
    Serial.print("SD Mounted Total: ");
    Serial.print(SD.totalBytes()/1024);
    Serial.print(" Used: ");
    Serial.print(SD.usedBytes()/1024);
    Serial.println(" bytes");  
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
    Serial.print("Create new data file:  ");
    Serial.println(fileName);
    Serial.println(Data);
  #endif
  if (SDPresent) {
    dataFile = SD.open(fileName, "w");
    #ifdef DEBUG_LOGGER
      Serial.println("SD");
    #endif
  }else{
    dataFile = LittleFS.open(fileName, "w");
    #ifdef DEBUG_LOGGER
      Serial.println("LittleFS"); 
    #endif
  }
  // add JSON structure to new file
  if (dataFile) {
      Serial.println(dataFile.name());
      dataFile.print("{\"Data\":[");
      dataFile.println();
      dataFile.print(Data);
      dataFile.print("]}");
      
      dataFile.flush();
      dataFile.close();
      result = true;
      #ifdef DEBUG_LOGGER
        Serial.println("Data added to new file"); 
      #endif  
  }
  
  //printFile(fileName);
  //Serial.println(dataFile.size());
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
  } 
  #ifdef DEBUG_LOGGER
    Serial.println(Data);
    Serial.println("Data file already exists");
  #endif
  // do not use "a" or "a+"" because seek don't work in this mode
  if (SDPresent) {
    dataFile = SD.open(fileName, "r+");
    #ifdef DEBUG_LOGGER
      Serial.println("SD PRESENT");
    #endif 
  }else{  
    dataFile = LittleFS.open(fileName, "r+");
    #ifdef DEBUG_LOGGER
      Serial.println("LittleFS PRESENT"); 
    #endif  
  } 
  
  if (dataFile) {
    Serial.println(dataFile.name());
    // erase "]}" at the end of the file
    Serial.println("Data file is opened ");
    //Serial.println(dataFile.size());
    dataFile.seek(dataFile.size() - 2); 
    // append data
    dataFile.print(",");
    dataFile.print(Data);
    dataFile.println();
    
    // rewrite closing "]}"
    dataFile.print("]}");
    dataFile.flush();
    //Serial.println(dataFile.size());
    dataFile.close();
    result = true;
    
    #ifdef DEBUG_LOGGER
      Serial.println("Data added to existing file");
      Serial.println(Data);
    #endif
  } else {
    #ifdef DEBUG_LOGGER
      Serial.println("Can not open existing file!");
    #endif  
  }
  Serial.println("saveOnSD return TRUE");
 // printFile(fileName);
  return result; 
}

void Logger::createFileName(){
// File name is based on Local Time 
  tm timeinfo;
  getLocalTime(&timeinfo);
  #ifdef DEBUG_LOGGER
    Serial.println("Create FileName");    
  #endif
  sprintf(fileName, "/%04d_%02d_%02d.JSN", 1900+timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  #ifdef DEBUG_LOGGER
    Serial.println(fileName); 
  #endif
}


bool Logger::prepareData(){
  createFileName();
  tm timeinfo;
  // data are based to UTC
  getUtcTime(&timeinfo);
  #ifdef DEBUG_LOGGER
    //printUtcTime();
  #endif
  sprintf(utc, "%04d-%02d-%02dT%02d:%02d:%02d", 1900+timeinfo.tm_year, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); // UTC time
 // Prepare json
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
  if (hasData) { // at least one data present
    #ifdef DEBUG_LOGGER
      Serial.print("Logger has data");
      Serial.println();
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
      Serial.println("[processLogger] Data saved on SD");
      Serial.print("[processLogger] Logged at local time  ");
      Serial.println(utc);
    #endif
    return true; 
  } else {
    return false;
  }
}

void Logger::processLogger() {
	struct tm LocalTimeinfo;
	getLocalTime(&LocalTimeinfo);
	if (LocalTimeinfo.tm_min % 15 == 0){
    if (!logged){
      //printLocalTime();
      if (prepareData()) {
				if( xSemaphoreTake( xSemaphore, (200 * portTICK_PERIOD_MS) == pdTRUE)){
					Serial.println("get xSemaphoreTake by Logger");
					if (logData()) {
						logged = true;
					}
					xSemaphoreGive(xSemaphore); 
					Serial.println("xSemaphoreGive by Logger");
					} else{ 
					Serial.println(" Unable to take xSemaphore");
					logged = false;
				} 
			}
			else {
				Serial.println("No Data available");
				logged = false;
			}	
		}  
	} else {
	  logged = false; 
	}  
}  


    

