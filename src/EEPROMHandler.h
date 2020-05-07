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
#define WIFI_CONNECTION_FLAG 0x50
#define BLUETOOTH_CONNECTION_FLAG 0x51
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
            EEPROM.writeByte(WIFI_CONNECTION_FLAG, 0);
            EEPROM.writeByte(BLUETOOTH_CONNECTION_FLAG, 0);
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

        void setWifiConnectionFlag(int val) {
            EEPROM.writeByte(WIFI_CONNECTION_FLAG, val);
            comitWrite();
        }

        void setBluetoothConnectionFlag(int val) {
            EEPROM.writeByte(BLUETOOTH_CONNECTION_FLAG, val);
            comitWrite();
        }

        int getWifiConnectionFlag() {
            return EEPROM.readByte(WIFI_CONNECTION_FLAG);
        }

        int getBluetoothConnectionFlag() {
            return EEPROM.readByte(BLUETOOTH_CONNECTION_FLAG);
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