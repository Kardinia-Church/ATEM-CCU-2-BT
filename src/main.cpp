#include <WiFi.h>
#include <WifiUdp.h>
#include "utility.h"
#include "settings.h"
#include "prefHandler.h"
#include "WiFi.h"
#include <DNSServer.h>
#include "webServer.h"

PreferencesHandler prefHandler;
DNSServer dnsServer;
WebServer webServer;
WiFiUDP udp;
bool APOpen = false;
int packetSize = 0;
byte packetBuffer[100];

#include "cameraHandler.h"
#include "tallyHandler.h"
CameraHandler cameraHandler;

#ifdef TALLY_FEATURE
TallyHandler tallyHandler;
#endif

//Open an AP to allow for configuration using the web ui
void openAP() {
  Serial.print("Opening AP on SSID: " + String(CONFIG_SSID) + "... ");
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(CONFIG_SSID);
  dnsServer.start(53, "*", IPAddress(192, 168, 1, 1));
  webServer.start(&prefHandler);
  Serial.println(" Ready");
  prefHandler.setRebootFlag(0);
  APOpen = true;
  tallyHandler.setUserLED(TALLY_COLOR_AP_MODE, false);
  tallyHandler.setStageLED(TALLY_COLOR_AP_MODE);
}

//Listen and handle serial events
int inMenu = -1;
void serialLoop(String input = "") {
  do {
    String inputString = input;
    while(Serial.available()) {
      inputString += (char)Serial.read();
    }

    //If we have an input
    if(inputString.length() > 0) {
      if(inMenu == -1) {
        inMenu = 0;
        Serial.println("\n------- Menu -------");
        Serial.println("1 - Change the WIFI settings");
        Serial.println("2 - Change the ATEM Connection Preference");
        Serial.println("3 - Change camera id");
        Serial.println("4 - Set the tally server IP");
        #ifdef TALLY_FEATURE
        Serial.println("5 - Set a list of ignored MEs");
        Serial.println("6 - Perform test on the tally");
        Serial.println("7 - Set user led brightness");
        Serial.println("8 - Set stage led brightness");
        #endif
        Serial.println("9 - Open AP to configuration tool");
        Serial.println("10 - Reset bluetooth pairing");
        Serial.println("11 - Reset EEPROM");
        Serial.println("12 - Reboot device");
        Serial.println("13 - Exit");
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
          //ATEM Settings
          case 2: {
            String connectionMode = "";
            String ip = "";
            Serial.println("Change ATEM Connection");
            Serial.println("Connection mode (0=Direct ATEM Connection, 1=Slave connection, 2=Node red connection): ");
            while(!Serial.available()) {}
            while(Serial.available()) {connectionMode += (char)Serial.read();}
            Serial.print(removeNewLine(connectionMode));
            Serial.println(" OK");
            Serial.print("IP: ");
            while(!Serial.available()) {}
            while(Serial.available()) {ip += (char)Serial.read();}
            Serial.print(removeNewLine(ip));
            Serial.println(" OK");
            prefHandler.writeATEMIP(removeNewLine(ip));
            prefHandler.setATEMConnectionMode(removeNewLine(connectionMode).toInt());
            inMenu = -1;
            ESP.restart();
            break;
          }
          case 3: {
            String id = "";
            Serial.println("Change Camera ID");
            Serial.println("Please enter the ID you would like to use for the camera: ");
            while(!Serial.available()) {}
            while(Serial.available()) {id += (char)Serial.read();}
            Serial.print(removeNewLine(id));
            Serial.println(" OK");
            prefHandler.writeCameraId(removeNewLine(id).toInt());
            inMenu = -1;
            ESP.restart();
            break;
          }
          #ifdef TALLY_FEATURE
          //Set the tally server ip
          case 4: {
            String ip = "";
            Serial.println("Change Tally Server IP Address");
            Serial.println("Please enter the IP address to the NodeRed server containing the tally handler: ");
            while(!Serial.available()) {}
            while(Serial.available()) {ip += (char)Serial.read();}
            Serial.print(removeNewLine(ip));
            Serial.println(" OK");
            prefHandler.writeTallyIP(removeNewLine(ip));
            inMenu = -1;
            ESP.restart();            
            break;
          }
          #endif
          #ifdef TALLY_FEATURE
          case 5: {
            String mes = "";
            Serial.println("Set the list of ignored MEs");
            Serial.println("Type the mes you'd like to ignore separated by a comma. Example: 1,2,3");
            while(!Serial.available()) {}
            while(Serial.available()) {mes += (char)Serial.read();}
            Serial.print(removeNewLine(mes));
            Serial.println(" OK");
            prefHandler.writeIgnoredMEs(removeNewLine(mes));
            inMenu = -1;
            ESP.restart(); 
            break;
          }
          //Set the tally server ip
          case 6: {
            String ip = "";
            Serial.println("Test the tally lights");
            Serial.println("Testing.. Please reboot to stop");
            tallyHandler.test();         
            break;
          }
          #endif
          case 7: {
            String val = "";
            Serial.println("Set the user LED brightness");
            Serial.println("Please enter a value between 0 - 100%");
            while(!Serial.available()) {}
            while(Serial.available()) {val += (char)Serial.read();}
            Serial.print(removeNewLine(val));
            Serial.println(" OK");
            prefHandler.writeUserBrightness(removeNewLine(val).toInt());
            inMenu = -1;
            ESP.restart();
            break;
          }
          case 8: {
            String val = "";
            Serial.println("Set the stage LED brightness");
            Serial.println("Please enter a value between 0 - 100%");
            while(!Serial.available()) {}
            while(Serial.available()) {val += (char)Serial.read();}
            Serial.print(removeNewLine(val));
            Serial.println(" OK");
            prefHandler.writeStageBrightness(removeNewLine(val).toInt());
            inMenu = -1;
            ESP.restart();
            break;
          }
          //Open AP to config tool
          case 9: {
            openAP();
            inMenu = -1;
            break;
          }
          //Reset bluetooth pairing
          case 10: {
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
          case 11: {
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
          case 12: {
            ESP.restart();
            break;
          }
          //Exit menu
          case 13: {
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


//Setup functions
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n\n\nATEM CCU 2 BT By Kardinia Church");
  Serial.println("Version: " + String(VERSION));
  Serial.println("Build Date: " + String(__DATE__));
  Serial.println("");
  Serial.print("Send anything over serial to open configuration menu.");
  prefHandler.initalize(); 
  #ifdef TALLY_FEATURE
  tallyHandler.begin(&prefHandler);
  #endif

  //Check if the device was rebooted twice. If so open the configuration tool
  if(prefHandler.getRebootFlag() >= 2) {
    Serial.println("Configuration tool requested. Opening AP.");
    openAP();
    return;
  }
  else {
    prefHandler.setRebootFlag(prefHandler.getRebootFlag() + 1);
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
  }
  
  prefHandler.setRebootFlag(0);

  //Start the wifi
  Serial.println("Starting STA");
  char ssid[32];
  char pass[32];
  prefHandler.readWifiSSID().toCharArray(ssid, 32);
  prefHandler.readWifiPassword().toCharArray(pass, 32);

  #ifdef TALLY_FEATURE
  tallyHandler.setUserLED(TALLY_COLOR_CONNECT);
  #endif

  //If the settings were not configured open the AP
  if(String(ssid) == "ssid" || String(pass) == "pass" || prefHandler.readATEMIP() == "0.0.0.0") {
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
        #ifdef TALLY_FEATURE
        tallyHandler.flashLEDBlocking(0, 0, 255, 200, 20);
        #endif
        ESP.restart();
        break;
      }
      delay(500);
    }
    
    Serial.print("Connected! IP address: "); Serial.println(WiFi.localIP());

    //Start the bluetooth
    cameraHandler.begin(prefHandler.getPref());

    if(cameraHandler.connect(&prefHandler)) {
      Serial.println(" Success");
    }
    else {
      Serial.println(" Failed");
       #ifdef TALLY_FEATURE
       tallyHandler.flashLEDBlocking(0, 255, 0, 200, 20);
       #endif
      openAP();
      return;
    }

    #ifdef TALLY_FEATURE
    Serial.print("Attempting to setup tally feature... ");
    if(tallyHandler.connect(prefHandler.getATEMConnectionMode())) {
      Serial.println("Success!");

      //Blink the led to show the camera ID
      if(prefHandler.getCameraId() != -1) {
        tallyHandler.setStageLED(0, 0, 0);
        delay(1000);
        tallyHandler.flashLEDBlocking(TALLY_COLOR_LIVE, 300, prefHandler.getCameraId());
        tallyHandler.flashLEDBlocking(TALLY_COLOR_STANDBY, 1000, 1);
      }
    }
    else {
      Serial.println(" Failed");
      #ifdef TALLY_FEATURE
      tallyHandler.flashLEDBlocking(255, 0, 0, 200, 20);
      #endif
      openAP();
      return;
    }
    #endif

    //Camera id
    if(prefHandler.getCameraId() == -1) {
      Serial.println("A camera ID was not set. Please set one!");
      #ifdef TALLY_FEATURE
      tallyHandler.flashLEDBlocking(255, 255, 0, 200, 20);
      #endif
      openAP();
      return;
    }
    else {
      Serial.println("Camera ID set to " + String(prefHandler.getCameraId()));
    }

    #ifdef TALLY_FEATURE
    tallyHandler.setStageLED(TALLY_COLOR_INDICATE);
    tallyHandler.setUserLED(TALLY_COLOR_BLACK);
    #endif
  }
}

//Main loop
void loop() {
  serialLoop();

  if(APOpen) {
    //Configuration AP is open handle connections
    dnsServer.processNextRequest();
    webServer.loop();
  }
  else {
    cameraHandler.loop();

    #ifdef TALLY_FEATURE
    tallyHandler.loop(cameraHandler.connected());
    #endif

    //If we lost connection to the wifi reboot
    if(WiFi.status() != WL_CONNECTED) {
      #ifdef TALLY_FEATURE
      tallyHandler.flashLEDBlocking(0, 0, 255, 200, 20);
      #endif
      ESP.restart();
    }
  }
}