#ifndef CONFIG_H_
#define CONFIG_H_

#define DEBUG_WIFI
#define DEBUG_LITTLEFS
#define DEBUG_FS
//#define DEBUG_DIRECTORY
#define DEBUG_LOGGER
//#define DEBUG_DATA_RECEIVED
//#define DEBUG_EVENTS_SEND
//#define DEBUG_PING
//#define DEBUG_LOAD_PEERS
#define DEBUG_SAVE_PEERS
//#define DEBUG_PRINT_PEERS
//#define DEBUG_DATA_STRUCTURE
//#define DEBUG_SETPOINTS
//#define DEBUG_WS_EVENTS
//#define DEBUG_PAIRING
//#define DEBUG_RSSI

//#define DEBUG_ON_SEND
//#define DEBUG_DATA_RECEIVED
//#define DEBUG_MEM
//#define DEBUG_TIME

// Replace with your network credentials (STATION)
static const char *ssid = "COGECO-BE360";
static const char *password = "3FEADCOGECO";
static const char *ntpServer = "pool.ntp.org";
static const char *timeZoneString = "EST5EDT,M3.2.0,M11.1.0";

//static const char *hydro = "http://ofsys.hydroquebec.com/T/OFSYS/SM3/375/2/S/F/8509/18499810/aPy66RR6.html";//2024

//const int GLOBAL_CONST_VAR = 0xFF;
//#define SERVER_TEST
#ifdef SERVER_TEST
   static const char *hostName = "servertest";
   static const int serverPort = 8080;
   #define SERVER_ID 99  
#else
   static const char *hostName = "server_2";
   static const int serverPort = 80;
   #define SERVER_ID 10
#endif

#endif