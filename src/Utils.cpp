#include "Utils.h"
#include "global.h"
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <print.h>
#include "config.h"
#include <arduino.h>
#include <UrlEncode.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>
#include "Credentials.h"
#include "setup.h"



SMTPSession smtp;

Preferences prefs;

esp_now_peer_info_t slave;
struct_ping pingData = {};

unsigned long elapsedTime = 0;

SemaphoreHandle_t serialMutex;

void startTime(){
  elapsedTime = millis();
}

void stopTime(){
  safeSerialPrintf("%d\n", millis() - elapsedTime);
}

void getUtcTime(struct tm * info){
    time_t now;
    time(&now);
    gmtime_r(&now, info);
}

void printUtcTime(){
  safeSerialPrintf("UTC Time : ");
  struct tm timeinfo;
  getUtcTime(&timeinfo);
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
  //safeSerialPrintf("%A, %B %d %Y %H:%M:%S zone %Z %z ", &timeinfo);
}

void printLocalTime(){
  safeSerialPrintf("Local Time : ");
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
  //safeSerialPrintf("%A, %B %d %Y %H:%M:%S zone %Z %z ", &timeinfo);
}

void checkMem()
{
#ifdef DEBUG_MEM
  safeSerialPrintf("--- Heap size : ");
  safeSerialPrintf("%d\n", ESP.getFreeHeap());
#endif
}

void showDirectory()
{
  #ifdef SERVER_TEST
    File root = LittleFS.open("/");
    safeSerialPrintf("LittleFS Card Directory");
  #else
    File root = SD.open("/");
    safeSerialPrintf("SD Card Directory");
  #endif
  File f = root.openNextFile();
  while (f)
  {
    safeSerialPrintf("FILE: ");
    safeSerialPrintf(f.name());
    safeSerialPrintf("  S=");
    safeSerialPrintf("%d\n", f.size());

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
    safeSerialPrintf("FILE: ");
    safeSerialPrintf(f.name());
    safeSerialPrintf("  S=");
    safeSerialPrintf("%d\n", f.size());
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

bool printFile(String fileName){
  #ifdef DEBUG_LOGGER_DATA
    File file = SD.open(fileName);
    if (!file) {
      safeSerialPrintf("Failed to open file for reading.");
      return false;
    }
    safeSerialPrintf("=================================\n");
    safeSerialPrintf("File content:\n");
    // Read and print the file content
    while (file.available()) {
      Serial.write(file.read());
    }
    safeSerialPrintf("\n=================================\n");
    // Close the file
    file.close();
  #endif  
  return true;
}
 

void sendMessageToWhatsapp(String message){
  String phoneNumber = phone;
  String apiKey = key;
  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    safeSerialPrintf("Message sent successfully\n");
  }
  else{
    safeSerialPrintf("Error sending the message\n");
    safeSerialPrintf("HTTP response code: %d\n", httpResponseCode);
  }

  // Free resources
  http.end();
}

bool checkWiFiChannel(){
  prefs.begin("WiFiChannel",false);
  int Old = prefs.getInt("channel",0);
  int New = WiFi.channel();
  bool changed = Old != New;
  prefs.putInt("channel",New);
  if(changed){  
    safeSerialPrintf("WiFi channel changed for %d to %d\n", Old, New);
  } 
  return changed;
}  





void sendLogJson() {

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASS;

  SMTP_Message message;
  message.sender.name = "ESP32 Logger";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Log ESP32 - JSON";
  message.addRecipient("Admin", RECIPIENT_EMAIL);

  message.text.content = "Veuillez trouver le log JSON en pièce jointe.";
  message.text.charSet = "utf-8";

  // 📎 Pièce jointe JSON
  /*SMTP_Attachment att;
  att.descr.filename = "log.json";
  att.descr.mime = "application/json";
  att.file.path = "/log.json";
  att.file.storage_type = esp_mail_file_storage_type_sd;

  message.addAttachment(att);*/

  if (!smtp.connect(&session)) {
    safeSerialPrintf("Erreur connexion SMTP\n");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message))
    safeSerialPrintf("Erreur envoi mail\n");
  else
    safeSerialPrintf("Log JSON envoyé avec succès\n ");
}



void safeSerialPrintf(const char *fmt, ...){
  if (!xSemaphoreTake(serialMutex, portMAX_DELAY)) return;
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.print(buf);
  xSemaphoreGive(serialMutex);
}
  






