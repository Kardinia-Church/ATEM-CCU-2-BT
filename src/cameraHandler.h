#include "Arduino.h"
#include "prefHandler.h"
#include "BlueMagic32/BlueMagic32.h"
#include "atemHandler.h"

class CameraHandler {
    private:
        PreferencesHandler *prefHandler;
        unsigned long lastUpdate;
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

            BMDControl = BMDConnection.connect();   
              
            atemBegin(prefHandler->readATEMIP(), 1);

            int count = 0;
            while(atemConnectionState != AtemConnectionState::Connected){
                Serial.print(".");
                delay(500);
                if(count++ > 10){return false;}
            }

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
            atemLoop();


            //Check connection


            
            if (BMDConnection.available()) {
                // if(BMDControl->timecodeChanged()) {
                //     Serial.println(BMDControl->timecode());
                //     lastUpdate = millis();
                // }
                // else if(lastUpdate + 5000 > millis()) {
                //     //Serial.println("Disconnect");
                // }
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