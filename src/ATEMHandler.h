#ifndef ATEM_HANDLER_H
#define ATEM_HANDLER_H

#include <WiFi.h>
#include <AsyncUDP.h>

#define ATEM_PORT 9910
#define ATEM_REQUEST_HANDSHAKE {0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define ATEM_HANDSHAKE_ACCEPTED {0x10, 0x14, 0x00, 0x00}
#define ATEM_HANDSHAKE_ANSWERBACK {0x80, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00}
#define ATEM_DISCONNECT {0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

enum AtemConnectionState {
    Disconnected,
    HandshakeRequestSent,
    GatheringInformation,
    Connected,
};

enum AtemPredefinedPacket {
    RequestHandshake,
    HandshakeAccepted,
    HandshakeAnswerback,
    Disconnect
};

AtemConnectionState atemConnectionState;
uint16_t atemLocalPacketId;
uint8_t atemSessionId[2];
int atemBufferLength;
AsyncUDPPacket *atemBuffer[50];                
AsyncUDP atemUdp;
unsigned long lastATEMResponse;

class ATEMHandler: public ATEMConnection {
    public: 
        //Send a predefined packet to the atem
        static void sendPredefinedPacket(AtemPredefinedPacket packet) {
            switch(packet) {
                case AtemPredefinedPacket::RequestHandshake: {
                    uint8_t buff[] = ATEM_REQUEST_HANDSHAKE;
                    buff[2] = highByte(random(1, 32767));
                    buff[3] = lowByte(random(1, 32767));
                    atemUdp.write(buff, sizeof(buff)/sizeof(uint8_t));
                    atemConnectionState = AtemConnectionState::HandshakeRequestSent;
                    break;
                }
                case AtemPredefinedPacket::HandshakeAnswerback: {
                    uint8_t buff[] = ATEM_HANDSHAKE_ANSWERBACK;
                    buff[2] = atemSessionId[0];
                    buff[3] = atemSessionId[1];
                    atemUdp.write(buff, sizeof(buff)/sizeof(uint8_t));
                    atemConnectionState = AtemConnectionState::GatheringInformation;
                    atemSessionId[0] = 0; atemSessionId[1] = 0;
                    break;
                }
                case AtemPredefinedPacket::Disconnect: {
                    break;
                }
                default: {
                    Serial.println("Unsupported packet sent");
                }
            }
        }

        //Begin
        bool begin(String ipAddress, PreferencesHandler *preferencesHandler) {
            prefHandler = preferencesHandler;
            atemIp = ipAddress;
            atemConnectionState = AtemConnectionState::Disconnected;
            atemLocalPacketId = 0;
            atemSessionId[0] = 0; atemSessionId[1] = 0;
            atemBufferLength = 0;
            lastATEMResponse = millis();

            IPAddress ip;
            ip.fromString(atemIp);
            Serial.print("Attempting connection to the ATEM @ " + atemIp);  
            if(atemUdp.connect(ip, ATEM_PORT)) {
                atemUdp.onPacket([](AsyncUDPPacket packet) {
                    

                    //Validate
                    int length = ((packet.data()[0] & 0x07) << 8) | packet.data()[1];
                    if(length == packet.length()) {
                        uint8_t flag = packet.data()[0] >> 3;
                        Serial.print(flag, HEX);

                        //Switch actions based on the connection state
                        switch(atemConnectionState) {
                            case AtemConnectionState::Disconnected: {
                                Serial.println("ERROR: We got a packet from the ATEM but we're disconnected?!");
                                break;
                            }
                            case AtemConnectionState::HandshakeRequestSent: {
                                atemLocalPacketId = 0;
                                atemSessionId[0] = packet.data()[2];
                                atemSessionId[1] = packet.data()[3];
                                sendPredefinedPacket(AtemPredefinedPacket::HandshakeAnswerback);
                                break;
                            }
                            case AtemConnectionState::Connected:
                            case AtemConnectionState::GatheringInformation: {
                                //Update our session id if we don't have one yet
                                if(atemSessionId[0] == 0 && atemSessionId[1] == 0) {
                                    atemSessionId[0] = packet.data()[2];
                                    atemSessionId[1] = packet.data()[3];
                                }
                                if(flag == 0x11) {
                                    atemConnectionState = AtemConnectionState::Connected;
                                }
                                atemBuffer[atemBufferLength] = &packet;
                                atemBufferLength++;
                                break;
                            }

                        }

                        //Send a reply to each message
                        uint8_t buffer[] = {0x80, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00};
                        buffer[2] = atemSessionId[0];
                        buffer[3] = atemSessionId[1];
                        buffer[4] = packet.data()[10];
                        buffer[5] = packet.data()[11];
                        atemUdp.write(buffer, 12);
                        lastATEMResponse = millis();
                    }
                });

                sendPredefinedPacket(AtemPredefinedPacket::RequestHandshake);

                int count = 0;
                unsigned long wait = millis();
                while(atemConnectionState != AtemConnectionState::Connected){
                    if(wait < millis()) {
                        Serial.print(".");
                        if(count++ > 10){Serial.println("Failed: Timeout"); return false;}
                        wait = millis() + 1000;
                    }
                }

                Serial.println("Success!");  
                return true;
            }
        }

        //Main loop
        byte *loop() {
            processATEMIncoming();

            if(lastATEMResponse + 5000 < millis() && atemConnectionState == AtemConnectionState::Connected) {
                Serial.println("ERROR: Disconnected from ATEM");
                atemConnectionState = AtemConnectionState::Disconnected;
                atemUdp.close();
                atemSessionId[0] = 0;
                atemSessionId[1] = 0;
                atemLocalPacketId = 0;
                atemBufferLength = 0;

                //Wait for a bit then attempt a reconnection
                if(lastATEMResponse + 10000 < millis()) {
                    begin(atemIp, prefHandler);
                }
            }
        }

    private:
        String atemIp = "0.0.0.0";
        PreferencesHandler *prefHandler;
        
        //Process the incoming ATEM packets looking for camera pamameters
        void processATEMIncoming() {
            // //Serial.println(atemBufferLength);
            // for(int i = 0; i < atemBufferLength; i++) {
            //                     //Serial.println("-1");
            //     if(atemBuffer[i]->length() > 12) {
            //         for(uint16_t j = 12; j < atemBuffer[i]->length();) {
            //             //Serial.println("0");
            //             uint16_t commandLength = atemBuffer[i]->data()[j] * 256 + atemBuffer[i]->data()[j + 1];

            //             //Serial.println("1");

            //             //Process the internal command
            //             String cmd = "";
            //             for(int k = j + 4; k < j + 8; k++) {
            //                 cmd += (char)atemBuffer[i]->data()[k];
            //             }

            //             //Serial.println("2");
            //             j+=commandLength;
                    
            //             //Process our commands
            //             if(cmd.compareTo("CCdP") == 0) {
            //                 Serial.println("CAMERA");
            //                 // for(int k = j + 8; k < atemBuffer[i]->length(); k++) {
            //                 //     Serial.print(atemBuffer[i]->data()[k], HEX);
            //                 // }
            //                 Serial.println();
            //             }
            //         }
            //     }
            // }
            // atemBufferLength = 0;
        }
};

#endif