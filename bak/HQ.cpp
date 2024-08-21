#include <HTTPClient.h>
#include "Utils.h"
//#include "config.h"
//#include "HQ.h"


/*String getHQValue(HTTPClient &http, String key, int skip, int get) {
  bool found = false, look = false;
  uint ind = 0;
  String ret_str = "";

  int len = http.getSize();
  char char_buff[1];
  WiFiClient * stream = http.getStreamPtr();
  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(char_buff, ((size > sizeof(char_buff)) ? sizeof(char_buff) : size));
      if (len > 0)
        len -= c;
      if (found) {
        if (skip == 0) {
          ret_str += char_buff[0];
          get --;
        } else
          skip --;
        if (get <= 0)
          break;
      }
      else if ((!look) && (char_buff[0] == key[0])) {
        look = true;
        ind = 1;
      } else if (look && (char_buff[0] == key[ind])) {
        ind ++;
        if (ind == key.length()) found = true;
      } else if (look && (char_buff[0] != key[ind])) {
        ind = 0;
        look = false;
      }
    }
  }
  return ret_str;
}*/

/*int8_t getMonth(String M){
    String months[12] = {"janvier","fevrier","mars","avril","mai","juin","juillet","aout","septembre","octobre","novembre","decembre"};  
    String R = "";
    for ( byte i = 0; i < 12; i++) {
        if (months[i] == M) {
           return i+1;
        }
    } 
    return 0;
}*/

/*bool isNew(JsonArray events, char* start, char* stop){
  bool found = false;
  for (JsonArray::iterator it=events.begin(); it!=events.end(); ++it) {
        if (((*it)["start"] = start) && ((*it)["stop"] = stop)) {
            found = true;
            #ifdef DEBUG_HQ
              Serial.println("already exists"); 
            #endif
            break;
        }
  }
  return !found;
}*/

/*void addEvent(JsonArray events,int8_t month, int8_t day, int8_t de, int8_t aa){
    setLocalTime();
    char start[20];
    sprintf(start,"%04d-%02d-%02dT%02d",year(local),month,day,de);
    char stop[20];
    sprintf(stop,"%04d-%02d-%02dT%02d",year(local),month,day,aa);
    Serial.print("T1 : ");
    Serial.println(start);
    Serial.print("T2 : ");
    Serial.println(stop);
    if (isNew(events, start, stop)){ 
      JsonObject event = events.createNestedObject();
      event["start"] = start;
      event["stop"] = stop;
    }
}*/

/*void cleanData(JsonArray events){
  setLocalTime();
  char now[20];
  sprintf(now,"%04d-%02d-%02dT%02d",year(local),month(local),day(local),hour(local));
  for (JsonArray::iterator it=events.begin(); it!=events.end(); ++it) {
    if ((*it)["stop"] < String(now)) {     
      events.remove(it);
      Serial.println("deleted");
    }
  }
}*/

/*void getHQData(JsonArray events) {
    Serial.println("getHQData");
    WiFiClient client;
    HTTPClient http;
    http.begin(client, hydro); 
    
    int httpCode = http.GET();     
    if (httpCode == 200){
      String A = "<li><b>";  
      String P = getHQValue(http, A, 0, 200);    
      int ndx1 = 0;
      int ndx2 = P.indexOf("</ul>") + 6;
      P = P.substring(ndx1,ndx2);
      P = "<li><b>" + P;
      #ifdef DEBUG_HQ
        Serial.println(httpCode);
        Serial.println(P);
      #endif  
      int n = 0;
      while (P.length() > 20)  {
        n++;
        ndx1 = P.indexOf("<li><b>") + 7;
        ndx2 = P.indexOf(" ");
        String day = P.substring(ndx1,ndx2);
        
        ndx1 = ndx2+1;
        ndx2 = P.indexOf("</b>");
        String month = P.substring(ndx1,ndx2);
      
        ndx1 = P.indexOf("&nbsp;de ")+9;
        ndx2 = P.indexOf("  h");
        String de = P.substring(ndx1,ndx2);
      
        P = P.substring(ndx2+4);
        ndx1 = P.indexOf("à&nbsp;")+8;
        ndx2 = P.indexOf("  h");
        String aa = P.substring(ndx1,ndx2);
        
        ndx1 = P.indexOf("</li>") + 5;
        P = P.substring(ndx1); 
        
     //   addEvent(events, getMonth(month), (int8_t)day.toInt(), (int8_t)de.toInt(), (int8_t)aa.toInt());
        #ifdef DEBUG_HQ
          char buf[64];
          sprintf(buf,"%02d-%02d de %02d a %02d", getMonth(month), (int)day.toInt(), (int)de.toInt(), (int)aa.toInt());
          Serial.println(buf);
        #endif
      }
    }
    http.end();
   // addEvent(events, 3, 24, 21, 22);
   // addEvent(events, 3, 25, 9, 12);
  }  
*/
/*void UpdateHQEvents(){
  Serial.println("UpdateHQEvents");
  StaticJsonDocument<2000> root;
  File file;
  file = LittleFS.open("/HQ.jsn","r");
  if (file) {  
    Serial.println("file is open");
    DeserializationError error  = deserializeJson(root, file); 
    Serial.println(error.c_str());
    if (!error) {
      serializeJsonPretty(root, Serial);
      getHQData(root["events"]);
     // cleanData(root["events"]);
    }
  }
  Serial.println("-------------------");
  serializeJsonPretty(root, Serial);
  file.close();
  file = LittleFS.open("/HQ.jsn","w");
  serializeJsonPretty(root,file);
  #ifdef DEBUG_HQ
    serializeJsonPretty(root,Serial);
  #endif  
  file.close();
}*/

/*void setHQStatus() {
  Serial.println("getHQStatus");
  setLocalTime();
  int8_t offset = 0;
  char now[20];
  sprintf(now,"%04d-%02d-%02dT%02d",year(local),month(local),day(local),hour(local));
  Serial.println(now);
  sprintf(now,"%04d-%02d-%02dT%02d",year(local-3600),month(local-3600),day(local-3600),hour(local-3600));
  Serial.println(now);
  StaticJsonDocument<2000> root;
  File file;
  file = LittleFS.open("HQ.jsn","r");
  if (file) {  
    auto error  = deserializeJson(root, file);  		
    if (!error) { 
      JsonArray events = root["events"];
      for (JsonArray::iterator it=events.begin(); it!=events.end(); ++it) {
        Serial.println("---");
        String start = (*it)["start"];
        String stop = (*it)["stop"];
        Serial.println(start);
        Serial.println(stop);
        if (String(now) >= start  && String(now) < stop ) {      
          offset = -2;
          break;
        } 
      }
    }
  } 
  file.close(); 
  OffsetHQ = offset;
  #ifdef DEBUG_HQ
     Serial.print(OffsetHQ);
  #endif
  
}*/
