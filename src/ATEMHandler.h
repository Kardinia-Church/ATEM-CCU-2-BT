#ifndef ATEM_HANDLER_H
#define ATEM_HANDLER_H

#include <WiFi.h>
#include <AsyncUDP.h>

#define ATEM_PORT 9910
#define ATEM_REQUEST_HANDSHAKE {0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define ATEM_HANDSHAKE_ACCEPTED {0x10, 0x14, 0x00, 0x00}
#define ATEM_HANDSHAKE_ANSWERBACK {0x80, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00}
#define ATEM_DISCONNECT {0x10, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define MAX_MES 10

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
bool atemCameraControlReadable = false;
byte atemCameraControlBuffer[24];
AsyncUDP atemUdp;
unsigned long lastATEMResponse;
bool atemLiveOn[MAX_MES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool atemPrevOn[MAX_MES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool atemIsLive = false;
bool atemIsPrev = false;
bool atemTallyChanged = false;
String atemIgnoredMEs = "";
int cameraId;

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
            lastATEMResponse = millis();
            cameraId = prefHandler->getCameraId();
            atemIgnoredMEs = prefHandler->getIgnoredMEs();

            IPAddress ip;
            ip.fromString(atemIp);
            Serial.print("Attempting connection to the ATEM @ " + atemIp);  
            if(atemUdp.connect(ip, ATEM_PORT)) {
                atemUdp.onPacket([](AsyncUDPPacket packet) {

                    //Validate
                    int length = ((packet.data()[0] & 0x07) << 8) | packet.data()[1];
                    uint8_t flag = packet.data()[0] >> 3;

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
                            atemConnectionState = AtemConnectionState::GatheringInformation;
                            break;
                        }
                        case AtemConnectionState::Connected:
                        case AtemConnectionState::GatheringInformation: {
                            //Update our session id if we don't have one yet
                            if(atemSessionId[0] == 0 && atemSessionId[1] == 0) {
                                atemSessionId[0] = packet.data()[2];
                                atemSessionId[1] = packet.data()[3];
                            }

                            atemIsLive = false;
                            atemIsPrev = false;

                            //Go though all the commands in the packet
                            uint16_t i = 12;
                            while(i < packet.length()) {
                                uint16_t commandLength = packet.data()[i] * 256 + packet.data()[i + 1];

                                //Individual command
                                String cmd = "";
                                for(uint16_t j = i + 4; j < i + 8; j++) {
                                    cmd += (char)packet.data()[j];
                                }

                                //If it's a camera control send it to the camera handler
                                if(cmd == "CCdP") {
                                    for(int k = 0; k < 24; k++) {
                                        atemCameraControlBuffer[k] = packet.data()[(i + 8) + k];
                                    }
                                    atemCameraControlReadable = true;
                                }

                                #ifdef TALLY_FEATURE
                                //Program input
                                if(cmd == "PrgI") {
                                    bool live = (((packet.data()[i + 10] << 8) | (packet.data()[i + 11] & 0xff)) == cameraId);
                                    atemLiveOn[packet.data()[i + 8]] = live;
                                    atemTallyChanged = true;
                                }

                                if(cmd == "PrvI") {
                                    bool live = (((packet.data()[i + 10] << 8) | (packet.data()[i + 11] & 0xff)) == cameraId);
                                    atemPrevOn[packet.data()[i + 8]] = live;
                                    atemTallyChanged = true;
                                }

                                if(cmd == "TrPs") {
                                    if(atemIgnoredMEs.indexOf(String(packet.data()[i + 8] + 1)) == -1) {
                                        if(packet.data()[i + 9] == 1) {
                                            if(atemPrevOn[packet.data()[i + 8]]) {
                                                atemLiveOn[packet.data()[i + 8]] = true;
                                                
                                                //Try not to spam the tally handler
                                                if(packet.data()[2] < 100){atemTallyChanged = true;}

                                            }
                                        }
                                    }
                                }

                                #endif

                                i += commandLength;
                            }

                            
                            for(int i = 0; i < MAX_MES; i++) {
                                if(atemLiveOn[i] && atemIgnoredMEs.indexOf(String(i)) == -1){atemIsLive = true;}
                                if(atemPrevOn[i] && atemIgnoredMEs.indexOf(String(i)) == -1){atemIsPrev = true;}

                                if(atemIsLive && atemIsPrev){break;}
                            }  
                            break;
                        }

                    }

                    //If we get a heart beat we're connected
                    if(flag == 0x11) {
                        atemConnectionState = AtemConnectionState::Connected;
                    }

                    //Send a reply to each message
                    uint8_t buffer[] = {0x80, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00};
                    buffer[2] = atemSessionId[0];
                    buffer[3] = atemSessionId[1];
                    buffer[4] = packet.data()[10];
                    buffer[5] = packet.data()[11];
                    atemUdp.write(buffer, 12);
                    lastATEMResponse = millis();
                });

                sendPredefinedPacket(AtemPredefinedPacket::Disconnect);
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

        bool connected() {
            return atemConnectionState == AtemConnectionState::Connected;
        }

        //Main loop
        byte *loop() {
            if(lastATEMResponse + 5000 < millis() && atemConnectionState == AtemConnectionState::Connected) {
                Serial.println("ERROR: Disconnected from ATEM");
                atemConnectionState = AtemConnectionState::Disconnected;
                atemUdp.close();
                atemSessionId[0] = 0;
                atemSessionId[1] = 0;
                atemLocalPacketId = 0;
                return nullptr;
            }
            else if(lastATEMResponse + 10000 < millis()) {
                begin(atemIp, prefHandler);
                Serial.println("Attempting reconnection");
            }
            else {
                #ifdef TALLY_FEATURE
                //Send tally updates
                if(atemTallyChanged) {
                    memset(packetBuffer, 0, sizeof(packetBuffer));
                    packetBuffer[0] = CMD_PROGRAM;
                    packetBuffer[1] = atemIsLive;
                    packetBuffer[2] = CMD_PREVIEW;
                    packetBuffer[3] = atemIsPrev;
                    packetSize = 4;
                    atemTallyChanged = false;
                }
                #endif

                if(atemCameraControlReadable) {
                    atemCameraControlReadable = false;
                    return atemCameraControlBuffer;
                }
                else {
                    return nullptr;
                }
            }
        }

    private:
        String atemIp = "0.0.0.0";
        PreferencesHandler *prefHandler;
};

#endif