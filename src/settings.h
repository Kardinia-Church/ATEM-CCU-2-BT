#ifndef SETTINGS_H
#define SETTINGS_H

#define VERSION 0.6

//General settings
#define SERIAL_BAUD 115200
#define DEBUG_LED 13
#define CONFIG_SSID "ATEM CCU 2 BT"
#define DEFAULT_WIFI_SSID "ssid"
#define DEFAULT_WIFI_PASS "pass"
#define DEFAULT_WEBUI_PASS "password"

#define INCOMING_PORT 9046
#define OUTGOING_PORT 9045
#define CMD_SUBSCRIBE_CCU 0xAD
#define CMD_SUBSCRIBE_TALLY 0xAF
#define CMD_PING 0xFA
#define CMD_PROGRAM 0x01
#define CMD_PREVIEW 0x02

//Bluetooth settings
#define BLUETOOTH_DEVICE_NAME "CCU2BT"

//Tally settings
#define TALLY_FEATURE
#ifdef TALLY_FEATURE
    #define IGNORED_MES (int []){1, 2, 3}

    #define TALLY_COUNT 2
    #define TALLY_PIN 26
    #define TALLY_TYPE  NEO_RGB + NEO_KHZ800

    #define TALLY_COLOR_BLACK 0, 0, 0
    #define TALLY_COLOR_INDICATE 0, 0, 0
    #define TALLY_COLOR_STANDBY 0, 255, 0
    #define TALLY_COLOR_LIVE 255, 0, 0
    #define TALLY_COLOR_VERSION 255, 255, 0
    #define TALLY_COLOR_AP_MODE 0, 255, 100
    #define TALLY_COLOR_CONNECT 255, 100, 0

    #define TALLY_LED_USER (int []){0}
    #define TALLY_LED_STAGE (int []){1}
#endif

#endif