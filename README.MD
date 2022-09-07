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


TODO:
  In case of server restart, it may use another WiFi channel, we need to add code for this situation. 


