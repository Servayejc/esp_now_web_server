#include "Utils.h"
#include "global.h"
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <print.h>

esp_now_peer_info_t slave;
struct_ping pingData = {};


void getUtcTime(struct tm * info){
    time_t now;
    time(&now);
    gmtime_r(&now, info);
}

void printUtcTime(){
  Serial.print("UTC Time : ");
  struct tm timeinfo;
  getUtcTime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void printLocalTime(){
  Serial.print("Local Time : ");
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");

}

void checkMem()
{
#ifdef DEBUG_MEM
  Serial.print(F("--- Heap size : "));
  Serial.println(ESP.getFreeHeap());
#endif
}

void showDirectory()
{
  #ifdef SERVER_TEST
    File root = LittleFS.open("/");
    Serial.println("LittleFS Card Directory");
  #else
    File root = SD.open("/");
    Serial.println("SD Card Directory");
  #endif  
  File f = root.openNextFile();
  while (f)
  {
    Serial.print("FILE: ");
    Serial.print(f.name());
    Serial.print("  S=");
    Serial.println(f.size());

    f = root.openNextFile();
  }
  root.close();
}

void copyLittleFStoSD()
{
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f)
  {
    Serial.print("FILE: ");
    Serial.print(f.name());
    Serial.print("  S=");
    Serial.println(f.size());
    String f2Name = "/";
    f2Name += f.name();
    File f2 = SD.open(f2Name,"w");
    size_t n;  
    uint8_t buf[128];
    while ((n = f.read(buf, sizeof(buf))) > 0) {
      f2.write(buf, n);
    }
    f2.close(); // done, close the destination file
    f = root.openNextFile();
  }
  root.close();
}



