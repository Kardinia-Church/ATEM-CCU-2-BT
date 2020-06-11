/**
 * Blackmagic CCU to Bluetooth Adaptor For ESP32
 * By Kardinia Church 2020
 * 
 * Tested on a XC3800 ESP32 development board from Jaycar
 * 
 * 
 * prefHandler.h
 * Used to define the allocation of memory for storing information
**/

#ifndef PREF_HANDLER_H
#define PREF_HANDLER_H

#include <Preferences.h>
#include "settings.h"

#define PREF_NAME "BMDBTMEMORY"

class PreferencesHandler {
    private:
        Preferences *pref;

        //Write a string to memory
        void writeString(const char *key, String val) {
            pref->begin(PREF_NAME, false);
            pref->putString(key, val);
            pref->end();
        }

        //Get a string from memory. Returns "" if not found
        String getString(const char *key) {
            String ret = "";
            pref->begin(PREF_NAME, false);
            ret = pref->getString(key, "");
            pref->end();
            return ret;
        }

        //Put a integer value into memory
        void writeInt(const char *key, int32_t val) {
            pref->begin(PREF_NAME, false);
            pref->putInt(key, val);
            pref->end();
        }

        //Get a integer from memory. Returns NULL if not found
        int32_t getInt(const char *key) {
            int32_t ret = -1;
            pref->begin(PREF_NAME, false);
            ret = pref->getInt(key, -1);
            pref->end();
            return ret;
        }

    public:
        //Return the pref object
        Preferences *getPref() {
            return pref;
        }

        //Initalize
        bool initalize() {
            pref = new Preferences();

            //Check if the memory is valid. If not reset it
            if(getInt("BMDMEMORYSET") != 1) {
                Serial.println("Memory was not valid. Resetting it");
                resetMemory();
            }

            return true;
        }

        //Set to default values
        void resetMemory() {
            pref->clear();
            writeString("wifiSSID", DEFAULT_WIFI_SSID);
            writeString("wifiPassword", DEFAULT_WIFI_PASS);
            writeString("webUIPassword", DEFAULT_WEBUI_PASS);
            writeString("atemIPAddr", "0.0.0.0");
            writeInt("wifiAttempts", 0);
            writeInt("btAttempts", 0);
            writeInt("BMDMEMORYSET", 1);
        }

        void setWifiConnectionAttempts(int val) {
            writeInt("wifiAttempts", val);
        }

        void setBluetoothConnectionAttempts(int val) {
            writeInt("btAttempts", val);
        }

        int getWifiConnectionAttempts() {
            return getInt("wifiAttempts");
        }

        int getBluetoothConnectionAttempts() {
            return getInt("btAttempts");
        }
        
        //Write the web ui password
        void writeWebUIPassword(String password) {
            writeString("webUIPassword", password);
        }

        //Return the web ui password from memory
        String readWebUIPassword() {
            return getString("webUIPassword");
        }
        
        //Write the wifi settings
        void writeWifiSettings(String ssid, String pass) {
            writeString("wifiSSID", ssid);
            writeString("wifiPassword", pass);
        }

        //Read the wifi SSID from memory
        String readWifiSSID() {
            return getString("wifiSSID");
        }

        //Read the wifi password from memory
        String readWifiPassword() {
            return getString("wifiPassword");
        }

        //Write the ATEM ip
        void writeATEMIP(String ip) {
            writeString("atemIPAddr", ip);
            Serial.println("HERE");
        }

        //Read the ATEM ip
        String readATEMIP() {
            return getString("atemIPAddr");
        }
};

#endif