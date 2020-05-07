# Blackmagic-CCU-Bluetooth
A ESP32 project to convert CCU parameters into a bluetooth emulation to control Blackmagic cameras via bluetooth

# How to use

## Limitations
Please connect the device to the same power source as the camera. Due to limitations there is currently no detection when the camera and device disconnect.

## Configuration Menu
Connect the device via USB to a serial monitor at 115200 baud and send anything (press enter) to enter the configuration menu.
Follow the prompts on the display to change device settings

## Current Bugs
- Clearing the bluetooth pairing is not working. To fix clear the flash using the ESP32 tool and reflash the firmware
- Need to detect wifi disconnects

# Special Thanks
This project is made possible by the following
- [SchoolPost](https://github.com/schoolpost/) for [BlueMagic32](https://github.com/schoolpost/BlueMagic32) which provides the bluetooth functionality of this project
- [SKAARHOJ](https://www.skaarhoj.com/) for the research and listing of the commands to interface with the ATEM