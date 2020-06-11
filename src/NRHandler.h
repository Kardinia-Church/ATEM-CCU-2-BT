/**
 * Blackmagic CCU to Bluetooth Adaptor For ESP32
 * By Kardinia Church 2020
 * 
 * Tested on a XC3800 ESP32 development board from Jaycar
 * 
 * 
 * NRHandler.h
 * Used to handle messages sent from the ATEM with Node Red being a middle man
**/

#ifndef NR_HANDLER_H
#define NR_HANDLER_H
#include <WiFi.h>
#include <WifiUdp.h>

#define CMD_SUBSCRIBE 0xAF

WiFiUDP udp;
IPAddress atemIp;
uint8_t camId = -1;

//Send a request to subscribe
void subscribe() {
  udp.beginPacket(atemIp, NR_PORT_OUTGOING);
  uint8_t buffer[2] = {CMD_SUBSCRIBE, camId};
  udp.write(buffer, 2);
  udp.endPacket();
}

void atemBegin(String ipAddress, uint8_t cameraId) {
    camId = cameraId;
    atemIp.fromString(ipAddress);
    udp.begin(NR_PORT_INCOMING);
    subscribe();
}

byte packetBuffer[100];
byte *atemLoop(){
    //Check if there is a incoming packet to handle
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        memset(packetBuffer, 0, sizeof(packetBuffer));
        udp.read(packetBuffer, packetSize);
        return packetBuffer;
    }

    return nullptr;
}
#endif