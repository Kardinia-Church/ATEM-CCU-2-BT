#ifndef NR_HANDLER_H
#define NR_HANDLER_H
#include <WiFi.h>
#include <WifiUdp.h>
#include "ATEMConnection.h"

#define CMD_SUBSCRIBE 0xAF
#define CMD_PING 0xFA
#define NR_PORT_OUTGOING 9045
#define NR_PORT_INCOMING 9046


class NRHandler: public ATEMConnection {
    private:
        WiFiUDP udp;
        IPAddress atemIp;
        byte packetBuffer[100];
        bool pingSent = false;
        unsigned long lastMessage = millis();

        //Send a request to subscribe
        void subscribe() {
            udp.beginPacket(atemIp, NR_PORT_OUTGOING);
            uint8_t buffer[1] = {CMD_SUBSCRIBE};
            udp.write(buffer, 1);
            udp.endPacket();
            lastMessage = millis();
        }

        //Send a ping request
        void ping() {
            pingSent = true;
            udp.beginPacket(atemIp, NR_PORT_OUTGOING);
            uint8_t buffer[1] = {CMD_PING};
            udp.write(buffer, 1);
            udp.endPacket();
            lastMessage = millis();
        }

    public:
        bool begin(String ipAddress) {
            atemIp.fromString(ipAddress);
            udp.begin(NR_PORT_INCOMING);
            subscribe();
            return true;
        }

        byte *loop(){
            //Check if there is a incoming packet to handle
            int packetSize = udp.parsePacket();
            if (packetSize > 0) {
                memset(packetBuffer, 0, sizeof(packetBuffer));
                udp.read(packetBuffer, packetSize);
                pingSent = false;
                lastMessage = millis();
                return packetBuffer;
            }

            //Send ping or resubscribe if required
            else if(lastMessage + 1000 < millis()) {
                if(pingSent) {
                    subscribe();
                    Serial.println("Disconnected from Server Sending A Subscribe Request");
                }
                else {
                    ping();
                }
            }

            return nullptr;
        }
};
#endif