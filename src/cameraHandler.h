#include "Arduino.h"
#include "prefHandler.h"
#include "BlueMagic32/BlueMagic32.h"
#include "utility.h"
#include "ATEMConnection.h"
#include "atemHandler.h"
#include "NRHandler.h"

class CameraHandler {
    private:
        PreferencesHandler *prefHandler;
        ATEMConnection *atemHandler;
        unsigned long lastUpdate;
        int connectionMode = 0;
        int cameraId = -1;
    public:
        bool connect(PreferencesHandler *preferencesHandler) {
            prefHandler = preferencesHandler;
            connectionMode = prefHandler->getATEMConnectionMode();

            switch(connectionMode) {
                case 0: {
                    //Direct ATEM connectiona
                    atemHandler = new ATEMHandler;
                    break;
                }
                case 1: {
                    //Slave connection
                    Serial.println("Slave connection not supported yet, sorry. Setting mode to node red mode");
                    prefHandler->setATEMConnectionMode(2);
                    return false;
                    break;
                }
                case 2: {
                    //Node red connection
                    atemHandler = new NRHandler;
                    break;
                }
                default: {
                    Serial.println(" Connection mode was not understood! " + connectionMode);
                    prefHandler->setATEMConnectionMode(2);
                    return false;
                }
            }
   
            //Check if the ATEM ip is set
            if(prefHandler->readATEMIP() == "0.0.0.0") {
                Serial.print(" ATEM IP is not set ");
                return false;
            }

            if(prefHandler->getCameraId() == -1) {
                Serial.print(" Camera ID is not set ");
                return false;
            }
            cameraId = prefHandler->getCameraId();

            if(!atemHandler->begin(prefHandler->readATEMIP(), prefHandler)) {return false;}
            BMDControl = BMDConnection.connect();   
            return true;
        }

        //Setup
        bool begin(Preferences *pref) {
            BMDConnection.begin(BLUETOOTH_DEVICE_NAME, pref);
            return true;
        };

        bool connected() {
            return atemHandler->connected() && BMDConnection.available();
        };

        //Main loop
        void loop() {
            byte *data = atemHandler->loop();
            if(data != nullptr) {
                if (BMDConnection.available()) {
                        if(data[0] != cameraId){return;}

                        switch(data[1]) {      
                            case 0: {
                                //Lens
                                switch(data[2]) {
                                    case 0: {
                                        //Focus
                                        uint16_t value = ((int16_t)((data[16] << 8) | (data[17] & 0xff))) / 2;
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], data[2], 128, 0,  value & 0xff, (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                    case 2: {
                                         //Aperture
                                        uint16_t value = (((float)((data[16] << 8) | (data[17] & 0xff)) - 3072.0) / 15360.0) * 2047;
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], 3, 128, 0,  value & 0xff, (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                    case 9: {
                                        //Zoom speed
                                        uint16_t value = (int16_t)((data[16] << 8) | (data[17] & 0xff));
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], data[2], 128, 0,  value & 0xff, (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                }
                                break;
                            }
                            case 1: {
                                //Video
                                switch(data[2]) {
                                    case 2: {
                                        //White Balance
                                        int16_t whiteBalance = (data[16] << 8) | (data[17] & 0xff);
                                        uint8_t send[12] = {255, 8, 0, 0, 1, 2, 2, 0, (whiteBalance & 0xff), (whiteBalance >> 8), 0, 0}; //Tint is 0 as the ATEM doesn't support it
                                        BMDControl->custom(send, 12);
                                        break;
                                    }
                                    case 5: {
                                        //Shutter speed
                                        int32_t value = -1;
                                        switch((uint16_t)((data[18] << 8) | (data[19] & 0xff))) {
                                            case 20000: {value = 50; break;}
                                            case 16667: {value = 60; break;}
                                            case 13333: {value = 75; break;}
                                            case 11111: {value = 90; break;}
                                            case 10000: {value = 100; break;}
                                            case 8333: {value = 120; break;}
                                            case 6667: {value = 150; break;}
                                            case 5556: {value = 180; break;}
                                            case 4000: {value = 250; break;}
                                            case 2778: {value = 360; break;}
                                            case 2000: {value = 500; break;}
                                            case 1379: {value = 725; break;}
                                            case 1000: {value = 1000; break;}
                                            case 690: {value = 1450; break;}
                                            case 500: {value = 2000; break;}
                                        }

                                        uint8_t send[12] = {255, 8, 0, 0, 1, 12, 3, 0, (value & 0xff), (value >> 8), 0, 0};
                                        BMDControl->custom(send, 12);
                                        break;
                                    }
                                    case 13: {
                                        //Gain
                                        uint8_t send[9] = {255, 5, 0, 0, data[1], data[2], 1, 0, data[16]};
                                        BMDControl->custom(send, 9);
                                        break;
                                    }
                                }
                                break;
                            }
                            case 8: {
                                switch(data[2]) {
                                    case 0: {
                                        //Lift
                                        uint16_t r = (int16_t)((data[16] << 8) | (data[17] & 0xff)) / 2;
                                        uint16_t g = (int16_t)((data[18] << 8) | (data[19] & 0xff)) / 2;
                                        uint16_t b = (int16_t)((data[20] << 8) | (data[21] & 0xff)) / 2;
                                        uint16_t y = (int16_t)((data[22] << 8) | (data[23] & 0xff)) / 2;
                                        uint8_t send[16] = {255, 12, 0, 0, data[1], data[2], 128, 0, (r & 0xff), (r >> 8), (g & 0xff), (g >> 8), (b & 0xff), (b >> 8), (y & 0xff), (y >> 8)};
                                        BMDControl->custom(send, 16);
                                        break;
                                    }
                                    case 1: {
                                        //Gamma
                                        uint16_t r = (int16_t)((data[16] << 8) | (data[17] & 0xff)) / 4;
                                        uint16_t g = (int16_t)((data[18] << 8) | (data[19] & 0xff)) / 4;
                                        uint16_t b = (int16_t)((data[20] << 8) | (data[21] & 0xff)) / 4;
                                        uint16_t y = (int16_t)((data[22] << 8) | (data[23] & 0xff)) / 4;
                                        uint8_t send[16] = {255, 12, 0, 0, data[1], data[2], 128, 0, (r & 0xff), (r >> 8), (g & 0xff), (g >> 8), (b & 0xff), (b >> 8), (y & 0xff), (y >> 8)};
                                        BMDControl->custom(send, 16);
                                        break;
                                    }
                                    case 2:
                                    {
                                        //Gain
                                        uint16_t r = (int16_t)((data[16] << 8) | (data[17] & 0xff));
                                        uint16_t g = (int16_t)((data[18] << 8) | (data[19] & 0xff));
                                        uint16_t b = (int16_t)((data[20] << 8) | (data[21] & 0xff));
                                        uint16_t y = (int16_t)((data[22] << 8) | (data[23] & 0xff));
                                        uint8_t send[16] = {255, 12, 0, 0, data[1], data[2], 128, 0, (r & 0xff), (r >> 8), (g & 0xff), (g >> 8), (b & 0xff), (b >> 8), (y & 0xff), (y >> 8)};
                                        BMDControl->custom(send, 16);
                                        break;
                                    }
                                    case 4: {
                                        //Contrast
                                        uint16_t pivot = (uint16_t)((data[16] << 8) | (data[17] & 0xff));
                                        uint16_t adjust = (uint16_t)((data[18] << 8) | (data[19] & 0xff));
                                        uint8_t send[12] = {255, 8, 0, 0, data[1], data[2], 128, 0, (pivot & 0xff), (pivot >> 8), (adjust & 0xff), (adjust >> 8)};
                                        BMDControl->custom(send, 12);
                                        break;
                                    }
                                    case 5: {
                                        //Lum mix
                                        uint16_t value = (uint16_t)((data[16] << 8) | (data[17] & 0xff));
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], data[2], 128, 0, (value & 0xff), (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                    case 6: {
                                        //Color adjust
                                        int16_t hue = (int16_t)((data[16] << 8) | (data[17] & 0xff));
                                        uint16_t saturation = (int16_t)((data[18] << 8) | (data[19] & 0xff));
                                        uint8_t send[12] = {255, 8, 0, 0, data[1], data[2], 128, 0, (hue & 0xff), (hue >> 8), (saturation & 0xff), (saturation >> 8)};
                                        BMDControl->custom(send, 12);
                                        
                                        break;
                                    }
                                    break;
                                }
                                break;
                            }
                        }
                }
            }
        };

        // //Clear the bond between the camera and esp
        void clearPairing() {        
            BMDConnection.clearPairing();
        }

        void clearPairing(Preferences *pref) {
            //pref->begin(BLUETOOTH_DEVICE_NAME, false);
            BMDConnection.begin(pref);
            BMDConnection.clearPairing();
            BMDControl = BMDConnection.connect();
        }
};