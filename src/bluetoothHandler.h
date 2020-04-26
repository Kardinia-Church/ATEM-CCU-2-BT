#ifndef BLUETOOTH_HANDELER_H
#define BLUETOOTH_HANDELER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define DEBUG

static BLEAdvertisedDevice* bluetoothCurentDevice;
// static BLERemoteCharacteristic* pRemoteCharacteristic;
static bool bluetoothAttemptConnection = false;;
class BluetoothHandler {
    private:
        enum Services {
            deviceInformation,
            cameraService,
        };
        static BLEUUID getServiceUUID(int uuidPos) {
            switch(uuidPos) {
                case 0: {
                    return BLEUUID("180A");
                }
                case 1: {
                    return BLEUUID("291D567A-6D75-11E6-8B77-86F30CA893D3");
                }
                default: {
                    Serial.println("Unknown service");
                    BLEUUID("");
                }
            }
        };
        enum Characteristics {
            outgoingCameraControl,
            incomingCameraControl,
            timeCode,
            cameraStatus,
            deviceName,
            protocolVersion,
            cameraManufacture,
            cameraModel
        };
        static BLEUUID getCharacteristicUUID(int uuidPos) {
            switch(uuidPos) {
                case 0: {
                    return BLEUUID("5DD3465F-1AEE-4299-8493-D2ECA2F8E1BB");
                }
                case 1: {
                    return BLEUUID(" B864E140-76A0-416A-BF30-5876504537D9");
                }
                case 2: {
                    return BLEUUID("6D8F2110-86F1-41BF-9AFB-451D87E976C8");
                }
                case 3: {
                    return BLEUUID("7FE8691D-95DC-4FC5-8ABD-CA74339B51B9");
                }
                case 4: {
                    return BLEUUID("FFAC0C52-C9FB-41A0-B063-CC76282EB89C");
                }
                case 5: {
                    return BLEUUID("8F1FD018-B508-456F-8F82-3D392BEE2706");
                }
                case 6: {
                    return BLEUUID("2A29");                   
               }
                case 7: {
                    return BLEUUID("2A24");                  
                }
                default: {
                    Serial.println("Unknown characteristic");
                    return BLEUUID("");
                }
            }
        };

        static void notifyCallback(
            BLERemoteCharacteristic* pBLERemoteCharacteristic,
            uint8_t* pData,
            size_t length,
            bool isNotify) {
                Serial.print("Notify callback for characteristic ");
                Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
                Serial.print(" of data length ");
                Serial.println(length);
                Serial.print("data: ");
                Serial.println((char*)pData);
            }

        //Client handler
        class ClientCallback : public BLEClientCallbacks {
            void onConnect(BLEClient* pClient) {
                #ifdef DEBUG
                    Serial.print("Client ");
                    Serial.print(pClient->getConnId());
                    Serial.println(" connected");
                #endif
            };

            void onDisconnect(BLEClient* pClient) {
                #ifdef DEBUG
                    Serial.print("Client ");
                    Serial.print(pClient->getConnId());
                    Serial.println(" disconnected");
                #endif
                //BLEDevice::getScan()->clearResults();
                //BLEDevice::getScan()->start(5, true);
            };
        };

        //Search for devices
        class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
            void onResult(BLEAdvertisedDevice advertisedDevice) {
                #ifdef DEBUG
                    Serial.print("BT Device: ");
                    Serial.println(advertisedDevice.toString().c_str());
                #endif

                if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(getServiceUUID(Services::cameraService))) {
                    #ifdef DEBUG
                        Serial.println("Found device");
                    #endif
                    BLEDevice::getScan()->stop();
                    bluetoothCurentDevice = new BLEAdvertisedDevice(advertisedDevice);
                    bluetoothAttemptConnection = true;
                }
            }
        }; 

        //Check if the device is valid if so add it
        bool checkDevice() {
            //If we found a device attempt to connect to it and add it to the device pool
            #ifdef DEBUG
                Serial.print("Attempting to connect to ");
                Serial.print(bluetoothCurentDevice->getAddress().toString().c_str());
            #endif

            BLEClient*  pClient = BLEDevice::createClient();
            pClient->setClientCallbacks(new ClientCallback());
            pClient->connect(bluetoothCurentDevice);

            if(!pClient->isConnected()) {Serial.println(" Failed!");} else{
                Serial.println(" Connected!");

                //Attempt to get the service
                BLERemoteService* deviceInformationService = pClient->getService(getServiceUUID(Services::deviceInformation));
                BLERemoteService* cameraService = pClient->getService(getServiceUUID(Services::cameraService));

                //Check if services are avaliable
                bool serviceNotFound = false;
                if(deviceInformationService != nullptr){
                    #ifdef DEBUG
                        Serial.println("Device information service found!");
                    #endif
                } else {serviceNotFound = true;}
                if(cameraService != nullptr){
                    #ifdef DEBUG
                        Serial.println("Camera service found!");
                    #endif
                } else {serviceNotFound = true;}
                if(serviceNotFound){
                    #ifdef DEBUG
                        Serial.println("One or more services not found :(");
                    #endif   
                }
                else {
                    #ifdef DEBUG
                        Serial.println("Getting device characteristics");
                    #endif 

                    BLERemoteCharacteristic *pRemoteCharacteristic = cameraService->getCharacteristic(BLEUUID(getCharacteristicUUID(Characteristics::cameraStatus)));
                    if(pRemoteCharacteristic == nullptr) {
                        #ifdef DEBUG
                            Serial.println("Failed to get camera manufacturer");
                        #endif
                    }
                    else if(pRemoteCharacteristic->canRead()){
                        //pRemoteCharacteristic->writeValue(0x01);
                        std::string value = pRemoteCharacteristic->readValue();
                        Serial.print("The characteristic value was: ");
                        Serial.println(value.c_str());
                    }

                }
            }

            return false;
        };
    public:
        //Initalize the bluetooth
        bool setup() {
            BLEDevice::init("");

            //Setup a device scanner to search for the device
            BLEScan* pBLEScan = BLEDevice::getScan();
            pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
            pBLEScan->setInterval(1349);
            pBLEScan->setWindow(449);
            pBLEScan->setActiveScan(true);
            pBLEScan->start(10, false);

            while(true) {
                if(bluetoothAttemptConnection) {
                    if(checkDevice()) {return true;}
                    else {bluetoothAttemptConnection = false;}
                }
                else {
                    Serial.println("Search again");
                    break;
                }
            }
            return true;
        };

        void loop() { 
        };
};

#endif