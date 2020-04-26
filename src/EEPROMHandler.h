/**
 * Blackmagic CCU to Bluetooth Adaptor For ESP32
 * By Kardinia Church 2020
 * 
 * Tested on a XC3800 ESP32 development board from Jaycar
 * 
 * 
 * EEPROMSettings.h
 * Used to define the allocation of memory for storing information
**/

#ifndef EEPROM_HANDLER_H
#define EEPROM_HANDLER_H

#include "EEPROM.h"

#define WIFI_ADDR 0x00 //65 bytes in size 32(ssid) + 0xFE + 32(password)
#define WEBUI_PASS_ADDR 0x41 //32 bytes
#define UUID_ADDR 0x62 //16 bytes
#define EEPROM_SIZE 512

class EEPROMHandler {
    private:
        //Generate the CRC
        const unsigned long crc_table[16] = { 
            0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 
            0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c, 
            0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 
            0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c 
        };
        unsigned long generateCRC() {
            unsigned long crc = ~0L;

            for (int index = 0 ; index < EEPROM_SIZE - 4  ; ++index) {
                crc = crc_table[(crc ^ EEPROM.readByte(index)) & 0x0f] ^ (crc >> 4);
                crc = crc_table[(crc ^ (EEPROM.readByte(index) >> 4)) & 0x0f] ^ (crc >> 4);
                crc = ~crc;
            }
            return crc;
        };

        //Write the CRC value to the end of memory
        void writeCRC() {
            EEPROM.writeULong(EEPROM_SIZE - 4, generateCRC());
        }

        //Check if the CRC is correct
        bool checkCRC() {
            return EEPROM.readULong(EEPROM_SIZE - 4) == generateCRC();
        }

        //Commit the write to EEPROM and update the CRC
        void comitWrite() {
            writeCRC();
            EEPROM.commit();
        }
    public:
        //Initalize the eeprom
        bool initalize() {
            return EEPROM.begin(EEPROM_SIZE);
        }

        //Check if the memory is valid. Returns true if it is
        bool checkMemory() {
            return checkCRC();
        };

        //Set the EEPROM to default values
        void resetMemory() {
            for(int i =0; i < EEPROM_SIZE; i++) {
                EEPROM.writeByte(i, 0xFF);
            }

            //Write default values
            EEPROM.writeString(WIFI_ADDR, "ssid");
            EEPROM.writeByte(WIFI_ADDR + 32, 0xFE);
            EEPROM.writeString(WIFI_ADDR + 33, "password");
            EEPROM.writeString(WEBUI_PASS_ADDR, "password");
            randomSeed(analogRead(A0));
            for(int i = 0; i < 16; i++) {
                EEPROM.writeByte(UUID_ADDR + i, random(0, 255));
            }
            comitWrite();
        }

        //Print the entire memory for debug
        void printMemory() {
            int j = 0;
            for(int i = 0; i < EEPROM_SIZE; i++) {
                j++;
                if(j > 5) {
                    Serial.print(",");
                    Serial.println(EEPROM.readByte(i), HEX);
                    j = 0;
                }
                else {
                    Serial.print(",");
                    Serial.print(EEPROM.readByte(i), HEX);
                }
            }
        }

        //Write the bluetooth device name that we connect to into memory
        void writeBluetoothDeviceName(String name) {
            //Not implemented
        }

        //Read the bluetooth device name from memory
        String readBluetoothDeviceName() {
            //Not implemented
            return "Hayden's iPhone";
        }

        //Write the bluetooth device pin to memory
        void writeBluetoothDevicePin(String pin) {
            //Not implemented
        }

         //Read the bluetooth device pin from memory
        String readBluetoothDevicePin() {
            //Not implemented
            Serial.println("type pin");
            String pin = "";
            while(!Serial.available()){}
            while(Serial.available()) {pin += (char)Serial.read();}
            return removeNewLine(pin);
        }

        //Generate a random UUID and write to memory
        void generateUUID() {
            //UUID_ADDR
            randomSeed(analogRead(A0));
            for(int i = 0; i < 16; i++) {
                EEPROM.writeByte(UUID_ADDR + i, random(0, 255));
            }
            comitWrite();
        }

        //Get the service UUID from memory
        String getUUID() {
            String ret = "";
            for(int i = 0; i < 4; i++) {ret += String(EEPROM.readByte(UUID_ADDR + 0 + i), HEX);}
            ret += '-';
            for(int i = 0; i < 2; i++) {ret += String(EEPROM.readByte(UUID_ADDR + 4 + i), HEX);}
            ret += '-';
            for(int i = 0; i < 2; i++) {ret += String(EEPROM.readByte(UUID_ADDR + 6 + i), HEX);}
            ret += '-';
            for(int i = 0; i < 2; i++) {ret += String(EEPROM.readByte(UUID_ADDR + 8 + i), HEX);}
            ret += '-';
            for(int i = 0; i < 6; i++) {ret += String(EEPROM.readByte(UUID_ADDR + 10 + i), HEX);}
            return ret;
        }

        //Write the web ui password
        void writeWebUIPassword(String password) {
            EEPROM.writeString(WEBUI_PASS_ADDR, password);
            comitWrite();
        }

        //Return the web ui password from memory
        String readWebUIPassword() {
            return EEPROM.readString(WEBUI_PASS_ADDR);
        }
        
        //Write the wifi settings
        void writeWifiSettings(String ssid, String pass) {
            EEPROM.writeString(WIFI_ADDR, ssid);
            EEPROM.writeByte(WIFI_ADDR + 32, 0xFE);
            EEPROM.writeString(WIFI_ADDR + 33, pass);
            comitWrite();
        }

        //Read the wifi SSID from memory
        String readWifiSSID() {
            if(EEPROM.readByte(WIFI_ADDR + 32) == 0xFE) {
                return EEPROM.readString(WIFI_ADDR);
            }
            else {
                return "";
            }
        }

        //Read the wifi password from memory
        String readWifiPassword() {
            if(EEPROM.readByte(WIFI_ADDR + 32) == 0xFE) {
                return EEPROM.readString(WIFI_ADDR + 33);
            }
            else {
                return "";
            }
        }
};

#endif