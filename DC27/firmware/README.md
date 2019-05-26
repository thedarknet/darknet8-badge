DarkNet badge code for DC 27

To set up an OTA
in ota.cpp set
OTA\_WIFI\_SSID = your access point
OTA\_WIFI\_PASSWORD = your access point password
OTA\_FIRMWARE\_UPGRADE\_URL = ip address of box with https server

running the server:
cd server\_certs
./run\_https\_server.sh

Replace hello-world.bin if you want
