#include "Arduino.h"
#include "prefHandler.h"
#include "BlueMagic32/BlueMagic32.h"
#include "utility.h"

#ifdef USE_ATEM_DIRECT
    #include "atemHandler.h"
#endif
#ifdef USE_NR_ATEM
    #include "NRHandler.h"
#endif

// struct ATEMCamera {
//     uint8_t id;
//     int16_t iris = 0;
//     int16_t focus = 0;
//     int16_t overallGain = 0;
//     int16_t whiteBalance = 0;
//     int16_t zoomSpeed = 0;

//     int16_t liftR = 0;
//     int16_t liftG = 0;
//     int16_t liftB = 0;
//     int16_t liftY = 0;

//     int16_t gammaR = 0;
//     int16_t gammaG = 0;
//     int16_t gammaB = 0;
//     int16_t gammaY = 0;

//     int16_t gainR = 0;
//     int16_t gainG = 0;
//     int16_t gainB = 0;
//     int16_t gainY = 0;

//     int16_t lumMix = 0;
//     int16_t hue = 0;
//     int16_t shutter = 0;
//     int16_t contrast = 0;
//     int16_t saturation = 0;
// };

class CameraHandler {
    private:
        PreferencesHandler *prefHandler;
        unsigned long lastUpdate;
        // ATEMCamera atemCamera;
    public:
        bool connect(PreferencesHandler *preferencesHandler) {
            prefHandler = preferencesHandler;
            prefHandler->setBluetoothConnectionAttempts(prefHandler->getBluetoothConnectionAttempts() + 1);
            
            //If we have failed to connect 5 times reset the pairing
            if(prefHandler->getBluetoothConnectionAttempts() > 5) {
                Serial.print(" Failed to connect 5 times ");
                prefHandler->setBluetoothConnectionAttempts(0);
                return false;
            }

            
            //Check if the ATEM ip is set
            if(prefHandler->readATEMIP() == "0.0.0.0") {
                Serial.print(" ATEM IP is not set ");
                return false;
            }
            atemBegin(prefHandler->readATEMIP(), 1);

            #ifdef USE_ATEM_DIRECT
                int count = 0;
                while(atemConnectionState != AtemConnectionState::Connected){
                    Serial.print(".");
                    delay(500);
                    if(count++ > 10){return false;}
                }
            #endif

            BMDControl = BMDConnection.connect();   
            prefHandler->setBluetoothConnectionAttempts(0);
            lastUpdate = millis();
            return true;
        }

        //Setup
        bool begin(Preferences *pref) {
            BMDConnection.begin(BLUETOOTH_DEVICE_NAME, pref);
            return true;
        };

        //Main loop
        void loop() {
            byte *data = atemLoop();
            if(data != nullptr) {
                if (BMDConnection.available()) {


                        Serial.print("CMD");
                        Serial.print(data[1], HEX);
                        Serial.print(",");
                        Serial.print(data[2], HEX);
                        Serial.println();
                        switch(data[1]) {      
                            case 0: {
                                //Lens
                                switch(data[2]) {
                                    case 0: {
                                        //Focus
                                        uint16_t value = (uint32_t)mapfValue((int16_t)((data[16] << 8) | (data[17] & 0xff)), 0, 65535, 0, 32767);
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], data[2], 128, 0,  value & 0xff, (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                    case 2: {
                                         //Aperture
                                        uint16_t value = mapFloat((float((data[16] << 8) | (data[17] & 0xff)) - 3072.0) / 15360.0);
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], 3, 128, 0,  value & 0xff, (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                    case 9: {
                                        //Zoom speed
                                        uint16_t value = (uint32_t)mapfValue((int16_t)((data[16] << 8) | (data[17] & 0xff)), -2048, 2048, -2048.0, 2048.0);
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
                                        int16_t tint = (data[17] << 8) | (data[18] & 0xff);
                                        Serial.println(tint);
                                        BMDControl->whiteBalance(whiteBalance, tint);
                                        break;
                                    }
                                    case 5: {
                                        //Shutter speed
                                        // uint32_t value = ((int)data[16] << 24) | ((int)data[17] << 16) | ((int)data[18] << 8) | ((int)data[19]);
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

                                        BMDControl->shutterSpeed(value);
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
                                        uint16_t r = (uint32_t)mapfValue((int16_t)((data[16] << 8) | (data[17] & 0xff)), -4096, 4096, -2047.0, 2047.0);
                                        uint16_t g = (uint32_t)mapfValue((int16_t)((data[18] << 8) | (data[19] & 0xff)), -4096, 4096, -2047.0, 2047.0);
                                        uint16_t b = (uint32_t)mapfValue((int16_t)((data[20] << 8) | (data[21] & 0xff)), -4096, 4096, -2047.0, 2047.0);
                                        uint16_t y = (uint32_t)mapfValue((int16_t)((data[22] << 8) | (data[23] & 0xff)), -4096, 4096, -2047.0, 2047.0);
                                        uint8_t send[16] = {255, 12, 0, 0, data[1], data[2], 128, 0, (r & 0xff), (r >> 8), (g & 0xff), (g >> 8), (b & 0xff), (b >> 8), (y & 0xff), (y >> 8)};
                                        BMDControl->custom(send, 16);
                                        break;
                                    }
                                    case 1: {
                                        //Gamma
                                        uint16_t r = (uint32_t)mapfValue((int16_t)((data[16] << 8) | (data[17] & 0xff)), -8192, 8192, -2047.0, 2047.0);
                                        uint16_t g = (uint32_t)mapfValue((int16_t)((data[18] << 8) | (data[19] & 0xff)), -8192, 8192, -2047.0, 2047.0);
                                        uint16_t b = (uint32_t)mapfValue((int16_t)((data[20] << 8) | (data[21] & 0xff)), -8192, 8192, -2047.0, 2047.0);
                                        uint16_t y = (uint32_t)mapfValue((int16_t)((data[22] << 8) | (data[23] & 0xff)), -8192, 8192, -2047.0, 2047.0);
                                        uint8_t send[16] = {255, 12, 0, 0, data[1], data[2], 128, 0, (r & 0xff), (r >> 8), (g & 0xff), (g >> 8), (b & 0xff), (b >> 8), (y & 0xff), (y >> 8)};
                                        BMDControl->custom(send, 16);
                                        break;
                                    }
                                    case 2:
                                    {
                                        //Gain
                                        uint16_t r = (uint32_t)mapfValue((uint16_t)((data[16] << 8) | (data[17] & 0xff)), 0, 32767, 0, 32767);
                                        uint16_t g = (uint32_t)mapfValue((uint16_t)((data[18] << 8) | (data[19] & 0xff)), 0, 32767, 0, 32767);
                                        uint16_t b = (uint32_t)mapfValue((uint16_t)((data[20] << 8) | (data[21] & 0xff)), 0, 32767, 0, 32767);
                                        uint16_t y = (uint32_t)mapfValue((uint16_t)((data[22] << 8) | (data[23] & 0xff)), 0, 32767, 0, 32767);
                                        uint8_t send[16] = {255, 12, 0, 0, data[1], data[2], 128, 0, (r & 0xff), (r >> 8), (g & 0xff), (g >> 8), (b & 0xff), (b >> 8), (y & 0xff), (y >> 8)};
                                        BMDControl->custom(send, 16);
                                        break;
                                    }
                                    case 3: {
                                        //Offset NOT SUPPORTED
                                        break;
                                    }
                                    case 4: {
                                        //Contrast
                                        uint16_t pivot = (uint32_t)mapfValue((uint16_t)((data[16] << 8) | (data[17] & 0xff)), 0, 4096, 0, 4096.0);
                                        uint16_t adjust = (uint32_t)mapfValue((uint16_t)((data[18] << 8) | (data[19] & 0xff)), 0, 4096, 0, 4096.0);
                                        uint8_t send[12] = {255, 8, 0, 0, data[1], data[2], 128, 0, (pivot & 0xff), (pivot >> 8), (adjust & 0xff), (adjust >> 8)};
                                        BMDControl->custom(send, 12);
                                        break;
                                    }
                                    case 5: {
                                        //Lum mix
                                        uint16_t value = (uint32_t)mapfValue((uint16_t)((data[16] << 8) | (data[17] & 0xff)), 0, 2048, 0, 2048.0);
                                        uint8_t send[10] = {255, 6, 0, 0, data[1], data[2], 128, 0, (value & 0xff), (value >> 8)};
                                        BMDControl->custom(send, 10);
                                        break;
                                    }
                                    case 6: {
                                        //Color adjust
                                        uint16_t hue = (uint32_t)mapfValue((int16_t)((data[16] << 8) | (data[17] & 0xff)), -2048, 2048, -2048, 2048.0);
                                        uint16_t saturation = (uint32_t)mapfValue((uint16_t)((data[18] << 8) | (data[19] & 0xff)), 0, 2048, 0, 2048.0);
                                        uint8_t send[12] = {255, 8, 0, 0, data[1], data[2], 128, 0, (hue & 0xff), (hue >> 8), (saturation & 0xff), (saturation >> 8)};
                                        BMDControl->custom(send, 12);
                                        break;
                                    }
                                    break;
                                }
                                break;
                            }
                        }



                        
                        //BMDControl->custom(send, 16);

                        


                   // Serial.println(String((data[16] << 8) | (data[17] & 0xff))  + ":" + String((data[18] << 8) | (data[19] & 0xff))  + ":" + String((data[20] << 8) | (data[21] & 0xff))  + ":" + String((data[22] << 8) | (data[23] & 0xff)));
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