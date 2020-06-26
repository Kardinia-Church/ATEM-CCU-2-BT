#ifndef NR_HANDLER_H
#define NR_HANDLER_H
#include "ATEMConnection.h"

class NRHandler: public ATEMConnection {
    private:
        PreferencesHandler *prefHandler;
        IPAddress atemIp;
        bool pingSent = false;
        unsigned long lastMessage = millis();

        //Send a request to subscribe
        void subscribe() {
            udp.beginPacket(atemIp, OUTGOING_PORT);
            uint8_t buffer[2] = {CMD_SUBSCRIBE_CCU, prefHandler->getCameraId()};
            udp.write(buffer, 2);
            udp.endPacket();
            lastMessage = millis();
        }

        //Send a ping request
        void ping() {
            pingSent = true;
            udp.beginPacket(atemIp, OUTGOING_PORT);
            uint8_t buffer[1] = {CMD_PING};
            udp.write(buffer, 1);
            udp.endPacket();
            lastMessage = millis();
        }

    public:
        bool begin(String ipAddress, PreferencesHandler *preferencesHandler) {
            prefHandler = preferencesHandler;
            atemIp.fromString(ipAddress);
            udp.begin(INCOMING_PORT);
            subscribe();
            return true;
        }

        bool connected() {
            return (lastMessage + 1000 < millis()) && pingSent;
        }

        byte *loop(){
            packetSize = udp.parsePacket();
            if(packetSize > 0) {
                memset(packetBuffer, 0, sizeof(packetBuffer));
                udp.read(packetBuffer, packetSize);
            }

            //Check if there is a incoming packet to handle
            if (packetSize == 24) {
                pingSent = false;
                lastMessage = millis();
                return packetBuffer;
            }

            //Handle pings
            else if(packetSize == 1) {
                if(packetBuffer[0] == CMD_PING) {
                    pingSent = false;
                    lastMessage = millis();
                }
            }

            //Send ping or resubscribe if required
            else if(lastMessage + 1000 < millis()) {
                if(pingSent) {
                    subscribe();
                    Serial.println("Disconnected from CCU Server Sending A Subscribe Request");
                }
                else {
                    ping();
                }
            }

            return nullptr;
        }
};
#endif