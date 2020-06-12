#ifndef statusIndicator_H
#define statusIndicator_H
#include "Arduino.h"

class StatusIndicator {
    public:
        enum Status {
            Waiting,
            Disconnected,
            Connecting,
            NormalOperation,
            APOpen,
            OtherError,
            NoChange
        };

        StatusIndicator(int pin) {
            ledPin = pin;
            pinMode(ledPin, OUTPUT);
            
        }
        //Update the status indicator
        void updateStatus(Status status = Status::NoChange) {
            if(status != Status::NoChange && status != currentState){currentState = status; flashCount = 0; waitMillis = millis();}

            if(waitMillis < millis()) {
                if(flashCount == 0) {
                   waitMillis = millis() + 4000; 
                   digitalWrite(ledPin, !digitalRead(ledPin));
                }
                else {
                    switch(currentState) {
                        case Status::Waiting: {if(flashCount > 5) {flashCount = 0;} waitMillis = millis() + 1000; break;}
                        case Status::Disconnected: {waitMillis = millis() + 200; break;}
                        case Status::Connecting:  {waitMillis = millis() + 500; break;}
                        case Status::NormalOperation: {waitMillis = millis() + 1000; break;}
                        case Status::APOpen:  {if(flashCount > 3) {flashCount = 0;} waitMillis = millis() + 500; break;}
                        case Status::OtherError:  {if(flashCount > 2) {flashCount = 0;} waitMillis = millis() + 500; break;}
                    }
                    digitalWrite(ledPin, !digitalRead(ledPin));
                }

                flashCount++;
            }
        }
    private:
        Status currentState = Status::Waiting;
        unsigned long waitMillis = millis();
        int flashCount = 0;
        int ledPin = 0;
 };
 #endif