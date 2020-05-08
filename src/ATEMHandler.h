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

struct ATEMCamera {
    uint8_t id;
    int16_t iris = 0;
    int16_t focus = 0;
    int16_t overallGain = 0;
    int16_t whiteBalance = 0;
    int16_t zoomSpeed = 0;

    int16_t liftR = 0;
    int16_t liftG = 0;
    int16_t liftB = 0;
    int16_t liftY = 0;

    int16_t gammaR = 0;
    int16_t gammaG = 0;
    int16_t gammaB = 0;
    int16_t gammaY = 0;

    int16_t gainR = 0;
    int16_t gainG = 0;
    int16_t gainB = 0;
    int16_t gainY = 0;

    int16_t lumMix = 0;
    int16_t hue = 0;
    int16_t shutter = 0;
    int16_t contrast = 0;
    int16_t saturation = 0;
};

ATEMCamera atemCamera;
uint8_t atemSessionId[2]; 
AtemConnectionState atemConnectionState;
AsyncUDP atemUdp;
uint16_t atemLocalPacketId = 0;
int atemBufferLength = 0;
AsyncUDPPacket* atemBuffer[50];
String atemIp = "0.0.0.0";

//Send a predefined packet to the atem
void sendPredefinedPacket(AtemPredefinedPacket packet) {
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

//Process the incoming ATEM packets looking for camera pamameters
void processATEMIncoming() {
    Serial.println(atemBufferLength);
    for(int i = 0; i < atemBufferLength; i++) {
                        //Serial.println("-1");
        if(atemBuffer[i]->length() > 12) {
            for(uint16_t j = 12; j < atemBuffer[i]->length();) {
                //Serial.println("0");
                uint16_t commandLength = atemBuffer[i]->data()[j] * 256 + atemBuffer[i]->data()[j + 1];

                //Serial.println("1");

                //Process the internal command
                String cmd = "";
                for(int k = j + 4; k < j + 8; k++) {
                    cmd += (char)atemBuffer[i]->data()[k];
                }

                //Serial.println(cmd);

                //Serial.println("2");
                j+=commandLength;
            }


            //Process our commands
            // if(cmd.compareTo("CCdP") == 0) {
            //     Serial.println("camera control");

            // }



        }
    }
    atemBufferLength = 0;
}

unsigned long lastATEMResponse = millis();

void atemBegin(String ipAddress, uint8_t cameraId) {
    atemCamera.id = cameraId;
    atemIp = ipAddress;
    atemConnectionState = AtemConnectionState::Disconnected;
    atemSessionId[0] = 0; atemSessionId[1] = 0;

    IPAddress ip;
    ip.fromString(atemIp);
    Serial.println("Attempting connection to the ATEM @ " + atemIp);  
    if(atemUdp.connect(ip, ATEM_PORT)) {
        atemUdp.onPacket([](AsyncUDPPacket packet) {

            //Validate
            int length = ((packet.data()[0] & 0x07) << 8) | packet.data()[1];
            if(length == packet.length()) {
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
    }
}
void atemLoop() {
    processATEMIncoming();

    if(lastATEMResponse + 5000 < millis() && atemConnectionState == AtemConnectionState::Connected) {
        uint8_t camId = atemCamera.id;
        Serial.println("ERROR: Disconnected from ATEM");
        atemConnectionState = AtemConnectionState::Disconnected;
        atemUdp.close();
        atemSessionId[0] = 0;
        atemSessionId[1] = 0;
        atemLocalPacketId = 0;
        atemBufferLength = 0;
        atemCamera = ATEMCamera();

        //Wait for a bit then attempt a reconnection
        if(lastATEMResponse + 10000 < millis()) {
            atemBegin(atemIp, camId);
        }
    }
}


#endif