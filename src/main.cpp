/**
 * Blackmagic CCU to Bluetooth Adaptor For ESP32
 * By Kardinia Church 2020
 * 
 * Tested on a XC3800 ESP32 development board from Jaycar
**/

#include "utility.h"
#include "settings.h"
#include "prefHandler.h"
#include "WiFi.h"
#include "cameraHandler.h"

CameraHandler cameraHandler;
PreferencesHandler prefHandler;
//DNSServer dnsServer;
bool inAPMode = false;

//Open an AP to allow for configuration using the web ui
void openAP() {
  Serial.print("Opening AP on SSID: " + String(CONFIG_SSID) + "... ");
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(CONFIG_SSID);
  //dnsServer.start(53, "*", IPAddress(192, 168, 1, 1));
  Serial.println(" Ready");
}

//Listen and handle serial events
int inMenu = -1;
void serialLoop() {
  do {
    String inputString = "";
    while(Serial.available()) {
      inputString += (char)Serial.read();
    }

    //If we have an input
    if(inputString.length() > 0) {
      if(inMenu == -1) {
        inMenu = 0;
        Serial.println("\n------- Menu -------");
        Serial.println("1 - Change the WIFI settings");
        Serial.println("2 - Open AP to configuration tool");
        Serial.println("3 - Reset bluetooth pairing");
        Serial.println("4 - Reset EEPROM");
        Serial.println("5 - Reboot device");
        Serial.println("6 - Exit");
      }
      else {
        if(inMenu == 0) {
          inMenu = inputString.toInt();
        }

        switch(inMenu) {
          case 0: {break;}
          //Wifi Settings
          case 1: {
            String ssid = "";
            String pass = "";
            Serial.println("Change Wifi Settings");
            Serial.print("SSID: ");
            while(!Serial.available()) {}
            while(Serial.available()) {ssid += (char)Serial.read();}
            Serial.println(removeNewLine(ssid));
            Serial.print("Password: ");
            while(!Serial.available()) {}
            while(Serial.available()) {pass += (char)Serial.read();}
            Serial.println("OK");
            prefHandler.writeWifiSettings(removeNewLine(ssid), removeNewLine(pass));
            inMenu = -1;
            ESP.restart();
            break;
          }
          //Open AP to config tool
          case 2: {
            openAP();
            inMenu = -1;
            break;
          }
          //Reset bluetooth pairing
          case 3: {
            Serial.println("Resetting the pairing will disconnect from the camera and forget it. Are you sure?\nType Y to erase or N to exit");
            while(!Serial.available()) {}
            String answer = "";
            while(Serial.available()) {answer += (char)Serial.read();}
            answer.toUpperCase();
            if(removeNewLine(answer) == "Y") {
              //cameraHandler.begin(prefHandler.getPref());
              prefHandler.initalize();
              cameraHandler.clearPairing(prefHandler.getPref());
              //cameraHandler.connect();
              Serial.println("Complete.");
              inMenu = -1;
            }
            else {
              inMenu = -1;
              Serial.println("Did not proceed");
            }
            break;
          }
          //Reset EEPROM
          case 4: {
            Serial.println("Resetting the EEPROM will erase ALL stored information including ALL settings! Are you sure?\nType Y to erase or N to exit");
            while(!Serial.available()) {}
            String answer = "";
            while(Serial.available()) {answer += (char)Serial.read();}
            answer.toUpperCase();
            if(removeNewLine(answer) == "Y") {
              prefHandler.resetMemory();
              Serial.println("Erased! Resetting");
              delay(1000);
              ESP.restart();
            }
            else {
              inMenu = -1;
              Serial.println("Did not proceed");
            }
            break;
          }
          //Reset
          case 5: {
            ESP.restart();
            break;
          }
          //Exit menu
          case 6: {
            Serial.println("");
            inMenu = -1;
            break;
          }
          default: {
            Serial.println("Unknown menu item");
            inMenu = 0;
            break;
          }
        }
      }
    }
  }  while(inMenu != -1);
}


void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n\n\nBlackmagic CCU Bluetooth Adaptor By Kardinia Church");
  Serial.println("Version :" + String(VERSION));
  Serial.println("Build Date: " + String(__DATE__));
  Serial.println("");
  Serial.print("Send anything over serial to open configuration menu.");

  //Allow the user to react
  int i = 0;
  while(true) {
    i++;
    serialLoop();
    Serial.print(".");
    if(i>10){break;}
    delay(500);
  }
  Serial.println("");

  prefHandler.initalize();

  //Check the memory
  Serial.print("Check memory... ");
  if(!prefHandler.checkMemory()) {
    Serial.println("Memory invalid. Resetting memory");
    prefHandler.resetMemory();
  }
  else {
    Serial.println(" Valid");
  }

  //Start the wifi
  Serial.println("Starting STA");
  char ssid[32];
  char pass[32];
  prefHandler.readWifiSSID().toCharArray(ssid, 32);
  prefHandler.readWifiPassword().toCharArray(pass, 32);

  //If the settings were not configured open the AP
  if(String(ssid) == "ssid" || String(pass) == "pass") {
    Serial.println("Memory was reset please set device settings");
    openAP();
  }
  else {
    Serial.print("Connecting to " + String(ssid) + "..");
    WiFi.begin(ssid, pass);

    //Wait till wifi is connected
    int i = 0;
    while (!WiFi.isConnected()) {
      Serial.print(".");
      serialLoop();
      i++;
      if(i > 50) {
        //Failed to connect
        Serial.println(" Failed");
        openAP();
        break;
      }
      delay(500);
    }
    
    Serial.print("Connected! IP address: "); Serial.println(WiFi.localIP());
    //setupServer();

    //Start the bluetooth
    cameraHandler.begin(prefHandler.getPref());
    cameraHandler.connect();
  }
}

void loop() {
  serialLoop();
  cameraHandler.loop();
}