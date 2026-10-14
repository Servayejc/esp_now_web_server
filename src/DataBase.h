#ifndef DATABASE_H_
#define DATABASE_H_

typedef struct SetPointEntry {
  uint8_t PeerID;
  uint8_t DeviceID;
  uint8_t SetPoint;
} SetPointEntry;

class DataBase  {
    public:
        typedef std::vector<PeerEntry> PeerList;
        
        DataBase();
        PeerList Peers;

        Preferences setpointsList;
        bool addSetPoint(uint8_t PeerID, uint8_t DeviceID, uint8_t SetPoint);
        void sendAllSetPoints();

        bool init(String structFilename, String peersFilename);
        bool addPeerToList(const uint8_t *mac_addr, int8_t PeerID);  // called by Serv
        bool addPeerToESPNOW(const uint8_t *peer_addr, uint8_t peerID);  // called by Main
        int macToPeerID(const uint8_t *mac_addr); // called by Serv
        uint8_t* PeerIDtoMAC(uint8_t PeerID);
        uint8_t getDeviceType(uint8_t PeerID, uint8_t DeviceID);
        void fillPairingData(uint8_t PeerID);
        void printSetPoints();
        void printPeersFromFile();
        void clearPeers();
    private: 

        bool fillDevices(String fileName);
        bool loadPeers(String peersFilename); 
        String peersFilename = "/peersList.js";
        void savePeers(String from);
        bool isEqualMAC(const uint8_t *mac_addr1, const uint8_t *mac_addr2);
        void testPeers(); // not used
};    

extern DataBase DB;



#endif