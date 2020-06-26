#include "settings.h"
#ifdef TALLY_FEATURE
    #ifndef TALLY_HANDLER_H
    #define TALLY_HANDLER_H

    //TODO add ignored mes to be read from pref



    #include "Arduino.h"
    #include "prefHandler.h"
    #include <Adafruit_NeoPixel.h>

    class TallyHandler {
        private:
            PreferencesHandler *prefHandler;
            Adafruit_NeoPixel *strip;
            IPAddress noderedIP;
            bool pingSent = false;
            unsigned long lastMessage = millis();
            bool programTally = false;
            bool previewTally = false;
            int atemConnectionMode = 0;
            int userBrightness = 100;
            int stageBrightness = 100;
                
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
            //Perform a test
            void test() {
                while(true) {
                    setUserLED(0, 0, 0);
                    setStageLED(0, 0, 0);
                    delay(500);
                    setUserLED(255, 0, 0);
                    setStageLED(0, 0, 0);
                    delay(500);
                    setUserLED(0, 255, 0);
                    setStageLED(0, 0, 0);
                    delay(500);
                    setUserLED(0, 0, 255);
                    setStageLED(0, 0, 0);
                    delay(500);
                    setUserLED(0, 0, 0);
                    setStageLED(255, 0, 0);
                    delay(500);
                    setUserLED(0, 0, 0);
                    setStageLED(0, 255, 0);
                    delay(500);
                    setUserLED(0, 0, 0);
                    setStageLED(0, 0, 255);
                    delay(500);
                }
            }

            //Set the user leds to a color
            void setUserLED(int r, int g, int b, bool show = true, int brightness = -1) {
                //If brightnes is set to -1 get it from memory
                if(brightness == -1) {brightness = userBrightness;}

                for(int i = 0; i < sizeof(TALLY_LED_USER)/sizeof(TALLY_LED_USER[0]); i++) {
                    strip->setPixelColor(TALLY_LED_USER[i], strip->Color((float)r * ((float)brightness/100), (float)g * ((float)brightness/100), (float)b * ((float)brightness/100)));
                }
                if(show){strip->show();}
            }

            //Set the stage leds to a color
            void setStageLED(int r, int g, int b, bool show = true, int brightness = 100) {
                if(brightness == -1) {brightness = stageBrightness;}
                
                for(int i = 0; i < sizeof(TALLY_LED_STAGE)/sizeof(TALLY_LED_STAGE[0]); i++) {
                    strip->setPixelColor(TALLY_LED_STAGE[i], strip->Color((float)r * ((float)brightness/100), (float)g * ((float)brightness/100), (float)b * ((float)brightness/100)));
                }
                if(show){strip->show();}
            }

            //Set the stage leds to a color
            void setLED(int index, int r, int b, int g, bool show = true, int brightness = -1) {
                strip->setPixelColor(index, strip->Color((float)r * ((float)brightness/100), (float)g * ((float)brightness/100), (float)b * ((float)brightness/100)));
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
                userBrightness = prefHandler->getUserBrightness();
                stageBrightness = prefHandler->getStageBrightness();
                for(int i = 0; i < TALLY_COUNT; i++) {setLED(i, TALLY_COLOR_BLACK, false);}
                setUserLED(TALLY_COLOR_VERSION);
                strip->show();
            }

            //Attempt to connect to the server
            bool connect(int atmConnectionMode) {
                atemConnectionMode = atmConnectionMode;
                if(atemConnectionMode == 0) {}
                else if(prefHandler->readTallyIP() == "0.0.0.0") {
                    Serial.print(" Tally IP not set");
                    return false;
                }
                else {
                    noderedIP.fromString(prefHandler->readTallyIP());
                    subscribe();
                }
            }

            void loop(bool atemConnected = false){
                if(atemConnectionMode == 0) {
                    if(atemConnected) {
                        lastMessage = millis();
                    }
                }

                //Check if there is a incoming packet to handle
                if (packetSize == 2 || packetSize == 4) {
                    pingSent = false;
                    lastMessage = millis();
                    int size = packetSize;

                    for(int i = 0; i <= (size / 2); i+=2) {
                        //Update based on tally information
                        switch(packetBuffer[i]) {
                            case CMD_PROGRAM: {
                                programTally = packetBuffer[i + 1] == 1;
                                break;
                            }
                            case CMD_PREVIEW: {
                                previewTally = packetBuffer[i + 1] == 1;
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

                    packetSize = 0;
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