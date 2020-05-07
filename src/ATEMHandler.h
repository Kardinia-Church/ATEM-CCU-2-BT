/**
 * Blackmagic CCU to Bluetooth Adaptor For ESP32
 * By Kardinia Church 2020
 * 
 * Tested on a XC3800 ESP32 development board from Jaycar
 * 
 * 
 * atemHandler.h
 * Used to handle communications with the ATEM
**/

#ifndef ATEM_HANDLER_H
#define ATEM_HANDLER_H

#include <WiFi.h>
#include <AsyncUDP.h>

#define ATEM_PORT 9910
#define ATEM_REQUEST_HANDSHAKE {0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define ATEM_HANDSHAKE_ACCEPTED {0x10, 0x14, 0x00, 0x00}
#define ATEM_HANDSHAKE_ANSWERBACK {0x80, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00}
#define ATEM_DISCONNECT {0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

enum ConnectionState {
    Disconnected,
    HandshakeRequestSent,
    GatheringInformation,
    Connected,
};

enum PredefinedPacket {
    RequestHandshake,
    HandshakeAccepted,
    HandshakeAnswerback,
    Disconnect
};

uint8_t sessionId[2]; 
ConnectionState connectionState;
AsyncUDP udp;
uint16_t localPacketId = 0;

//Send a predefined packet to the atem
void sendPredefinedPacket(PredefinedPacket packet) {
    switch(packet) {
        case PredefinedPacket::RequestHandshake: {
            uint8_t buff[] = ATEM_REQUEST_HANDSHAKE;
            buff[2] = highByte(random(1, 32767));
            buff[3] = lowByte(random(1, 32767));
            udp.write(buff, sizeof(buff)/sizeof(uint8_t));
            connectionState = ConnectionState::HandshakeRequestSent;
            break;
        }
        case PredefinedPacket::HandshakeAnswerback: {
            uint8_t buff[] = ATEM_HANDSHAKE_ANSWERBACK;
            buff[2] = sessionId[0];
            buff[3] = sessionId[1];
            udp.write(buff, sizeof(buff)/sizeof(uint8_t));
            connectionState = ConnectionState::GatheringInformation;
            sessionId[0] = 0; sessionId[1] = 0;
            break;
        }
        case PredefinedPacket::Disconnect: {
            break;
        }
        default: {
            Serial.println("Unsupported packet sent");
        }
    }
}

int atemBufferLength = 0;
AsyncUDPPacket *atemBuffer[50];

//Process the incoming ATEM packets looking for camera pamameters
void processATEMIncoming() {

    for(int i = 0; i < atemBufferLength; i++) {
        Serial.println("HERE");
        AsyncUDPPacket *packet = atemBuffer[i];
        Serial.println("HERE 2");

        uint16_t j = 12;
        while(packet->length() > 12) {
            if(j >= packet->length() - 1) {break;}
            uint16_t commandLength = packet->data()[j] * 256 + packet->data()[j + 1];
            i+=commandLength;
            Serial.println(i);
        }
        atemBufferLength--;
    }
}

unsigned long lastATEMResponse = millis();

class ATEMHandler {
    private:
        IPAddress ip;
        const int port = ATEM_PORT;
    public:
        void begin(String ipAddress) {
            connectionState = ConnectionState::Disconnected;
            sessionId[0] = 0; sessionId[1] = 0;

            ip.fromString(ipAddress);
            if(udp.connect(IPAddress(10,4,10,12), port)) {
                udp.onPacket([](AsyncUDPPacket packet) {

                    //Validate
                    int length = ((packet.data()[0] & 0x07) << 8) | packet.data()[1];
                    if(length == packet.length()) {
                        uint8_t flag = packet.data()[0] >> 3;

                        //Switch actions based on the connection state
                        switch(connectionState) {
                            case ConnectionState::Disconnected: {
                                Serial.println("ERROR: We got a packet from the ATEM but we're disconnected?!");
                                break;
                            }
                            case ConnectionState::HandshakeRequestSent: {
                                Serial.println("Got handshake with ATEM. Gathering information...");
                                localPacketId = 0;
                                sessionId[0] = packet.data()[2];
                                sessionId[1] = packet.data()[3];
                                sendPredefinedPacket(PredefinedPacket::HandshakeAnswerback);
                                break;
                            }
                            case ConnectionState::GatheringInformation: {
                                //Update our session id if we don't have one yet
                                if(sessionId[0] == 0 && sessionId[1] == 0) {
                                    sessionId[0] = packet.data()[2];
                                    sessionId[1] = packet.data()[3];
                                }
                                if(flag == 0x02) {
                                    connectionState = ConnectionState::Connected;
                                    Serial.println("Connected to ATEM");
                                }
                                atemBuffer[++atemBufferLength] = &packet;
                                break;
                            }
                            case ConnectionState::Connected: {
                                lastATEMResponse = millis();
                                atemBuffer[++atemBufferLength] = &packet;
                                break;
                            }
                        }
                        Serial.println("HI");

                        //Send a reply to each message
                        uint8_t buffer[] = {0x80, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00};
                        buffer[2] = sessionId[0];
                        buffer[3] = sessionId[1];
                        buffer[4] = packet.data()[10];
                        buffer[5] = packet.data()[11];
                        udp.write(buffer, 12);
                    }
                });

                sendPredefinedPacket(PredefinedPacket::RequestHandshake);
            }
        }
        void loop() {
            processATEMIncoming();


            if(lastATEMResponse + 5000 > millis()) {
                Serial.println("ERROR: Disconnected from ATEM");
            }

            // if(udp.available()) {
            //     Serial.println(udp)
            // }
        }
};

#endif