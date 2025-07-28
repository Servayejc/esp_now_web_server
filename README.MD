Hitron modem settings

- set port forwarding
	VESTA	80~80	80~80	TCP	192.168.0.100	Any
- set DHCP
	VESTA   192.168.0.100   Server MAC Address	




Description du projet

But du projet : 
	Fournir un serveur dédié à collecter les données de différents modules et les présenter dans un tableau de bord.
	Le serveur est accessible via WiFi en local.
	Le serveur communique avec les différents modules via le protocole ESP_NOW en se basant sur les addresses MAC.
	Les communications sont bi-directionnelles.
	Le pairage des modules se fait de façon entièrement automatique.
	Chaque module peut avoir plusieur fonctions différentes.
	La structure du réseau sera déterminée par un fichier JSON à partir du serveur.
	Les données seront traitées et enregistrées sur une carte mémoire microSD.
	Le serveur doit avoir un ID (1..255) 
	
Serveur 
	Build on ESP32 module. It is used in mode(WIFI_AP_STA);
	STA is used to connect to WiFi and ESP-NOW sender
		It is connected to WiFi channel selected by the router.
		All peers MUST uses the same WiFi channel for ESP-NOW communications.
		STA uses WiFi.macAddress for sending ESP-NOW messages to peers.
	AP uses WiFi.softAPmacAddress to receive ESP-NOW messages from peers.
	A micro SD card is uses to log data.
	HTML code is saved on LittleFS and generated automatically from a configuration JSON file.  
	The Server and all peers have a unique ID, the server ID must be known by all peers.    	 
	The server add en entry with the broadcast address and the channel to his peer list

Peers
	Build on ESP8266.
	All functions are implemented in each peer

	Each peer need server ID and a unique peerID (hard coded)
	Each peer implement an auto  connect function 
	Each peer send his data to server at some interval
	Each peer can receive data from server (setpoints, status...)

AutoConnect procedure
	The server is supposed running.
	At start-up, peer don't need access to the router and don't know the routeur MAC Address
	The pairing status is set to Pair_Request 
	Peer set ESP_NOW on channel 1
	The peer send a pairing request in broadcast 
	The pairing status is set to Pair_Requested   
	If the server receive the message :
		The channel is the good one (same as channel of the server) 
		It find the peer MAC address in the sender message
		It respond to the sender in unicast with a message containing his WiFi.softAPmacAddress
		If the peer receive the response:
			It add the MAC address of the server in his peer list
			It set the pairing status to Pair_Paired

	If there is no response from the server, repeat the process with next WiFi channel
		(if the WiFi channel is greater of 11 (13 in Europe), repeat process with channel 1) 

	This procedure is done once time and takes about 200 ms if the server is on line, else 
	this process continue until the server is present.  

-----------------------------------------------	 	
TODO

Save peers on SD and NOT on LittelFS

Adjust web page for different screes size

 	
	
	
		
	

 

