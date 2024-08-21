

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
	Il est bâti sur un module ESP32 programmé en C++ et d'un lecteur de carte microSD.
	Le code HTML de base est enregistré dans le ESP32 et est généré automatiquement au chargement de la page web.
	Le configuration est un fichier JSON qui sera lu au démarrage pour compléter la page web.
	Ce fichier DOIT avoir le même nom que la Page web. 
	
Modules
	Les modules (Peer) sont bâtis sur des ESP8266.
	Ils supportent toutes les fonctions (max 12) disponibles définies dans le fichier de configuration JSON du serveur.	
	Le module (Peer) doit avoir un ID unique (1..255).
	Le module doit connaitre le ID du serveur.   
	Au démarrage, le module va se conncter automatiquement au serveur (Voir documention AutoConnect).
	Il pourra recevoir des messages provenant du serveur (Valeur de consigne d'un thermostat, allumage d'une lampe ... ).
	Il va envoyer ses données (mesures, état d'une lampe...) au serveur au rythme défini dans le fichier de configuration du serveur.  	
	Il va exécuter les tâches nécessaires pour les fonctions choisies. 

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

If the server receive the message, we are on the good channel
    
    The server add the received MAC to his peer list 
    The server reply to the MAC address a message containing his softAPmacAddress and channel number

    The sender replace the broadcast address by the server address in his peer list

else 
    The sender repeat the process on the next channel

	
	
	
	
		
	

 

