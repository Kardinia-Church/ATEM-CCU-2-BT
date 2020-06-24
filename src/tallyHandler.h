#include "settings.h"
#ifdef TALLY_FEATURE
    #ifndef TALLY_HANDLER_H
    #define TALLY_HANDLER_H

    #include "Arduino.h"
    #include "prefHandler.h"
    #include <Adafruit_NeoPixel.h>

    #define CMD_PROGRAM 0x01
    #define CMD_PREVIEW 0x02

    class TallyHandler {
        private:
            PreferencesHandler *prefHandler;
            Adafruit_NeoPixel *strip;
            IPAddress noderedIP;
            bool pingSent = false;
            unsigned long lastMessage = millis();
            bool programTally = false;
            bool previewTally = false;
                
            //Send a request to subscribe
            void subscribe() {
                setUserLED(TALLY_COLOR_CONNECT, false);
                setStageLED(TALLY_COLOR_BLACK);
                udp.beginPacket(noderedIP, OUTGOING_PORT);
                uint8_t buffer[sizeof(IGNORED_MES)/sizeof(IGNORED_MES[0]) + 2] = {CMD_SUBSCRIBE_TALLY, prefHandler->getCameraId()};
                for(int i = 0; i < sizeof(IGNORED_MES)/sizeof(IGNORED_MES[0]); i++) {
                    buffer[i + 2] = IGNORED_MES[i];
                }
                udp.write(buffer, sizeof(IGNORED_MES)/sizeof(IGNORED_MES[0]) + 2);
                udp.endPacket();
                lastMessage = millis();
            }

            //Send a ping request
            void ping() {
                pingSent = true;
                udp.beginPacket(noderedIP, OUTGOING_PORT);
                uint8_t buffer[1] = {CMD_PING};
                udp.write(buffer, 1);
                udp.endPacket();
                lastMessage = millis();
            }
        public:
            //Set the user leds to a color
            void setUserLED(int r, int g, int b, bool show = true) {
                for(int i = 0; i < sizeof(TALLY_LED_USER)/sizeof(TALLY_LED_USER[0]); i++) {
                    strip->setPixelColor(TALLY_LED_USER[i], strip->Color(r, g, b));
                }
                if(show){strip->show();}
            }

            //Set the stage leds to a color
            void setStageLED(int r, int g, int b, bool show = true) {
                for(int i = 0; i < sizeof(TALLY_LED_STAGE)/sizeof(TALLY_LED_STAGE[0]); i++) {
                    strip->setPixelColor(TALLY_LED_STAGE[i], strip->Color(r, g, b));
                }
                if(show){strip->show();}
            }

            //Set the stage leds to a color
            void setLED(int index, int r, int b, int g, bool show = true) {
                strip->setPixelColor(index, strip->Color(r, g, b));
                if(show){strip->show();}
            }

            //Flash the user led using blocking
            void flashLEDBlocking(int r, int g, int b, int del = 1000, int times = 1) {
                for(int i = 0; i < times; i++) {
                    setUserLED(r, g, b);
                    delay(del);
                    setUserLED(0, 0, 0);
                    delay(del);
                }
            }

            void begin(PreferencesHandler *preferencesHandler) {
                prefHandler = preferencesHandler;
                strip = new Adafruit_NeoPixel(TALLY_COUNT, TALLY_PIN, TALLY_TYPE);
                strip->begin();
                for(int i = 0; i < TALLY_COUNT; i++) {setLED(i, TALLY_COLOR_BLACK, false);}
                setUserLED(TALLY_COLOR_VERSION);
                strip->show();
            }

            //Attempt to connect to the server
            bool connect() {
                //Try to set IP from memory
                if(prefHandler->readTallyIP() == "0.0.0.0") {
                    Serial.print(" Tally IP not set");
                    return false;
                }
                else {
                    noderedIP.fromString(prefHandler->readTallyIP());
                    subscribe();
                }
            }

            void loop(){
            //Check if there is a incoming packet to handle
            if (packetSize == 2) {
                pingSent = false;
                lastMessage = millis();

                //Update based on tally information
                switch(packetBuffer[0]) {
                    case CMD_PROGRAM: {
                        programTally = packetBuffer[1] == 0x01;
                        break;
                    }
                    case CMD_PREVIEW: {
                        previewTally = packetBuffer[1] == 0x01;
                        break;
                    }
                }

                if(programTally) {
                    setUserLED(TALLY_COLOR_LIVE, false);
                    setStageLED(TALLY_COLOR_LIVE);
                }
                else if(previewTally) {
                    setUserLED(TALLY_COLOR_STANDBY, false);
                    setStageLED(TALLY_COLOR_INDICATE);
                }
                else {
                    setUserLED(TALLY_COLOR_BLACK, false);
                    setStageLED(TALLY_COLOR_INDICATE);
                }
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
                    Serial.println("Disconnected from Tally Server Sending A Subscribe Request");
                }
                else {
                    ping();
                }
            }
        }
    };

    #endif
#endif