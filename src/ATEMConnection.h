#ifndef ATEMConnection_H
#define ATEMConnection_H
#include "Arduino.h"

class ATEMConnection {
 public:
     virtual bool begin(String ipAddress, PreferencesHandler *preferencesHandler);
     virtual byte *loop();
 };
 #endif