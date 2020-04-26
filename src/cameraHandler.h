#include "Arduino.h"
#include "bluetoothHandler.h"

class CameraHandler {
    private:
        BluetoothHandler *bluetoothHandler;
    public:
        bool setup() {
            bluetoothHandler->setup();
            return true;
        };
        void loop() {
            bluetoothHandler->loop();
        };
};