#include "Arduino.h"
#include "EEPROMHandler.h"
#include "BlueMagic32/BlueMagic32.h"

#include <ATEMbase.h>
#include <ATEMext.h>

class CameraHandler {
    private:
        EEPROMHandler eepromHandler;
        unsigned long lastUpdate;
    public:
        bool connect() {
            eepromHandler.setBluetoothConnectionFlag(eepromHandler.getBluetoothConnectionFlag() + 1);
            
            //If we have failed to connect 5 times reset the pairing
            if(eepromHandler.getBluetoothConnectionFlag() > 5) {
                Serial.print(" Failed to connect 5 times ");
                eepromHandler.setBluetoothConnectionFlag(0);
                return false;
            }

            //BMDConnection.begin(BLUETOOTH_DEVICE_NAME);
            BMDControl = BMDConnection.connect();
            
            //If we're connected
            if(true) {
                eepromHandler.setBluetoothConnectionFlag(0);
                lastUpdate = millis();
                return true;
            }
            
            return false;
        }

        //Setup
        bool begin(Preferences *pref) {
            BMDConnection.begin(BLUETOOTH_DEVICE_NAME, pref);
            return true;
        };

        //Main loop
        void loop() {
            //Check connection

            if(BMDControl->timecodeChanged()) {
                Serial.println(BMDControl->timecode());
                lastUpdate = millis();
            }
            else if(lastUpdate + 5000 > millis()) {
                Serial.println("Disconnect");
            }

            
            if (BMDConnection.available()) {
                //Serial.println("connection");
                if(BMDControl->changed()) {
                    
                    Serial.print("ISO: "); Serial.println(BMDControl->getIso());
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