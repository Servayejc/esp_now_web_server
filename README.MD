JC Servaye
Complete project details at https://github.com/Servayejc/esp_now_sender/

Automatic pairing for ESP NOW

In the server: 
- The WiFi channel is determined by your router
- Don't know the sender MAC
- In WIFI_AP_STA mode respond whith his WiFi.macAddress
    but it the peer try to send data to server, we need to use the server WiFi.softAPmacAddress
- WiFi.softAPmacAddress if create by ESP32 in WIFI_AP_STA  

In the senders: 
- No access to the router
- Don't know the routeur MAC

The server is supposed running.

The sender set esp now on channel 1
The server add en entry with the broadcast address and the channel to his peer list
The sender send a pairing request in broadcast

If the server received the message, we are on the good channel
    
    The server add the received MAC to his peer list 
    The server reply to the MAC address a message containing his softAPmacAddress and channel number

    The sender replace the broadcast address by the server address in his peer list

else 
    The sender repeat the process on the next channel

Notes: 
  -In the code below, we use the size of the message to distinct regular message to pairing message
  -Uncomment //#define SAVE_CHANNEL to save the channel in EEPROM 
   but this has not a major impact of the pairing time 
  -13 channels exists in Europe, you can change MAX_CHANNEL value to accomodate your location  

TODO:
  In case of server restart, it may use another WiFi channel, we need to add code for this situation. 




ESP32 in WIFI_AP_STA mode respond whith his WiFi.macAddress but it uses WiFi.softAPmacAddress to receive data from a peer.
  example : 
      1.- The peer send a message to the server with address ff:ff:ff:ff:ff:ff 
      2.- The server receive the message and the mac Address of the peer.
      3.- The server add the address of the peer to his peer list. 
      4.- The server reply to the peer whith the received address. 
      5.- The peer receive the message and the WiFi.macAddress of the server.
      6.- The peer add the received address of the server to his peer list.
      7.- The peer try to send a message to the server address but it fail to transmit !!!
      8.- The peer add the WiFi.softAPmacAddress of the server to his peer list.
      9.- The peer send a message to the server WiFi.softAPmacAddress... 
      10.- The server receive the message from the peer!
  
  Don't trust the macAddress in the OnDataRecv(...) callback if the message is received 
  from an ESP in WIFI_AP_STA mode.  
  
in my case :    
 server macAddress          78:e3:6d:09:ee:84       Send data with this MAC      STA 
 server softAPmacAddress    78:e3:6d:09:ee:85       Receive data with this MAC   AP
