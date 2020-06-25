# ATEM CCU 2 BT
A ESP32 project to convert CCU parameters into a bluetooth emulation to control Blackmagic cameras via bluetooth wirelessly!

# Still in development
This project is still in development so it is missing certain features and has some bugs.

## What's working?
So far this project supports:
- Controlling CCU parameters using a Node Red server as a middle man to transfer the packets
- Serial configuration
- Tallying

This is now running on our wireless camera rig with great success!

## What's not working?
At the moment the following is not working or not finished:
- A direct ATEM connection
- A slave mode to connect to another device that is in direct connection mode acting as the master
- Web configuration
- Clearing a connected camera acts funky sometimes. If you have this issue try using the ESP32 tool to clear all memory
- Changing paramaters on the camera will not update the ATEM. It's only ATEM->Camera at the moment
- If bluetooth disconnects it may not reconnect

# Installation 
In order to use the project a ESP32 is required, this project was built on a ESP-WROOM-32 from Jaycar.

- Please make sure the device is powered from the same power source as the camera as it may not reconnect when the camera is power cycled (through mains)

- Download or clone the project files from Github
- Open the project in [PlatformIO](https://platformio.org/) using [Microsoft's Visual Studio Code](https://code.visualstudio.com/)
- Select your device's COM port and upload!

- Next connect to the device from a serial monitor on 115200 baud or use the wifi network to configure the device (currently not supported)
- If using the web configuration tool set all the values required.

- If using the serial monitor send anything to open the menu
- Set 1 (Change the WIFI settings) and 2 (Change the ATEM Connection Preference) as a minimum to setup the device following the prompts
- Once this is complete the device will attempt to find the camera via bluetooth please keep the camera near to the device
- A prompt should come up asking for a pin. This is displayed in the camera's bluetooth settings please enter it and hit enter.
- Hopefully the device will reboot and connect to everything!
- The camera id is set in the camera's settings under the setup page

## Hardware
### Tally feature
If desired one can connect a string of NeoPixels (we use 2 PL9823's) with a stage led and user led to show the tally connected to pin ```G26```

### 3D Files
Included are 3D printable files for both the tally lights and controller. The tally light file can be printed for two PL9823s to be used as a user led and stage led. The controller file can be used just to enclose your ESP32 with mounting points or to be used for the tally system.

## Node Red
If using the node red mode add the following to your flow to process the commands using the [blackmagic-atem-nodered](https://github.com/haydendonald/blackmagic-atem-nodered) module
```
[{"id":"4fee6af6.b85134","type":"udp in","z":"4f43af6.468e15","name":"","iface":"","port":"9045","ipv":"udp4","multicast":"false","group":"","datatype":"buffer","x":160,"y":520,"wires":[["ecfdb1f3.eb40d"]]},{"id":"e9d254b0.69cdc8","type":"atem-atem","z":"4f43af6.468e15","name":"Auditorium ATEM","network":"c8cac07b.eda62","outputMode":"supported","sendTime":"no","sendInitialData":"yes","sendStatusUpdates":"yes","x":350,"y":560,"wires":[["fcc3f0b6.0277b","75f6bbc8.0cb574"]]},{"id":"fcc3f0b6.0277b","type":"function","z":"4f43af6.468e15","name":"Send out CCU","func":"if(msg.payload.cmd == \"cameraControl\") {\n    if(msg.payload.raw !== undefined) {\n        //Command starts at pos 8\n        var cmd = msg.payload.raw.packet.slice(8);\n        \n        var msgs = [];\n        for(var i in flow.get(\"btBlackmagicSubscriptionsCCU\")) {\n            if(flow.get(\"btBlackmagicSubscriptionsCCU\")[i].inputId == cmd[0]) {\n                msgs.push({\n                    \"ip\": i,\n                    \"payload\": cmd\n                });                    \n            }\n        }\n        return [msgs];\n    }\n}","outputs":1,"noerr":0,"x":540,"y":560,"wires":[["b2bc6545.519bd8"]]},{"id":"ecfdb1f3.eb40d","type":"function","z":"4f43af6.468e15","name":"Subscribe","func":"//Add the tally to the subscriptions\nif(msg.ip !== undefined && msg.payload !== undefined) {\n    if(msg.payload[0] == 0xAD) {\n        //CCU subscriptions\n        flow.get(\"btBlackmagicSubscriptionsCCU\")[msg.ip] = {\n            \"ip\": msg.ip,\n            \"inputId\": msg.payload[1]\n        };\n        return [{\"ip\": msg.ip, \"payload\": new Buffer.from([0xFA])}, undefined];\n    }\n    else if(msg.payload[0] == 0xAF) {\n        //Tally subscriptions\n        flow.get(\"btBlackmagicSubscriptionsTally\")[msg.ip] = {\n            \"ip\": msg.ip,\n            \"inputId\": msg.payload[1],\n            \"ignoreMEs\": []\n        };\n        for(var i = 2; i < msg.payload.length; i++) {\n            if(msg.payload[i] !== 255) {\n                flow.get(\"btBlackmagicSubscriptionsTally\")[msg.ip].ignoreMEs.push(msg.payload[i]);\n            }\n        }\n        return [undefined, {\"payload\": {\"cmd\": \"tally\", \"data\": {}}}];\n    }\n    \n    var pings = [];\n    for(var i in flow.get(\"btBlackmagicSubscriptionsCCU\")) {\n        pings.push({\"ip\": i, \"payload\": new Buffer.from([0xFA])});\n    }\n    for(var i in flow.get(\"btBlackmagicSubscriptionsTally\")) {\n        pings.push({\"ip\": i, \"payload\": new Buffer.from([0xFA])});\n    }\n    return [pings, undefined];\n}","outputs":2,"noerr":0,"x":320,"y":520,"wires":[["b2bc6545.519bd8"],["e9d254b0.69cdc8"]]},{"id":"b2bc6545.519bd8","type":"udp out","z":"4f43af6.468e15","name":"","addr":"","iface":"","port":"9046","ipv":"udp4","outport":"","base64":false,"multicast":"false","x":720,"y":520,"wires":[]},{"id":"35d76ad7.f74b66","type":"function","z":"4f43af6.468e15","name":"Reset","func":"flow.set(\"btBlackmagicSubscriptionsCCU\", {});\nflow.set(\"btBlackmagicSubscriptionsTally\", {});","outputs":1,"noerr":0,"x":330,"y":480,"wires":[[]]},{"id":"c57fd684.63b578","type":"inject","z":"4f43af6.468e15","name":"","topic":"","payload":"","payloadType":"date","repeat":"","crontab":"","once":true,"onceDelay":0.1,"x":190,"y":480,"wires":[["35d76ad7.f74b66"]]},{"id":"a571b8ac.0d8778","type":"comment","z":"4f43af6.468e15","name":"BTBlackmagic","info":"","x":170,"y":420,"wires":[]},{"id":"75f6bbc8.0cb574","type":"function","z":"4f43af6.468e15","name":"Send out tally","func":"if(msg.payload.cmd == \"tally\") {\n    var msgs = [];\n    for(var i in flow.get(\"btBlackmagicSubscriptionsTally\")) {\n        var isProg = false;\n        var isPrev = false;  \n        \n        //Prog\n        for(var j in msg.payload.data[flow.get(\"btBlackmagicSubscriptionsTally\")[i].inputId].program.mes) {\n            var me = parseInt(msg.payload.data[flow.get(\"btBlackmagicSubscriptionsTally\")[i].inputId].program.mes[j]);\n            if(flow.get(\"btBlackmagicSubscriptionsTally\")[i].ignoreMEs.includes(me) === false) {isProg = true; break;}\n        }\n        //Check in transition\n        for(var j in msg.payload.data[flow.get(\"btBlackmagicSubscriptionsTally\")[i].inputId].inTransition.mes) {\n            var me = parseInt(msg.payload.data[flow.get(\"btBlackmagicSubscriptionsTally\")[i].inputId].inTransition.mes[j]);\n            if(flow.get(\"btBlackmagicSubscriptionsTally\")[i].ignoreMEs.includes(me) === false) {isProg = true; break;}\n        }\n        //Prev\n        for(var j in msg.payload.data[flow.get(\"btBlackmagicSubscriptionsTally\")[i].inputId].preview.mes) {\n            var me = parseInt(msg.payload.data[flow.get(\"btBlackmagicSubscriptionsTally\")[i].inputId].preview.mes[j]);\n            if(flow.get(\"btBlackmagicSubscriptionsTally\")[i].ignoreMEs.includes(me) === false) {isPrev = true; break;}\n        }\n        msgs.push({\n            \"ip\": flow.get(\"btBlackmagicSubscriptionsTally\")[i].ip,\n            \"payload\": new Buffer.from([0x01, isProg])\n        });\n        msgs.push({\n            \"ip\": flow.get(\"btBlackmagicSubscriptionsTally\")[i].ip,\n            \"payload\": new Buffer.from([0x02, isPrev])\n        }); \n    }\n        \n    return [msgs];\n        \n}","outputs":1,"noerr":0,"x":540,"y":600,"wires":[["b2bc6545.519bd8"]]},{"id":"c8cac07b.eda62","type":"atem-network","z":"","name":"ATEM","ipAddress":"192.168.0.1"}]
```

# How to use
The device once configured should connect and act as expected. If not the device can be configured using the following methods:
- ```Web Configuration``` Press the restart button 3 times to open a AP which when connected to can be used to configure the device. If not taken to the page navigate to ```192.168.1.1``` CURRENTLY NOT SUPPORTED!
- ```Serial Configuraion``` Connect to the device via USB and open a serial terminal at ```115200```. Once connected sending anything will open the configuration menu.

# Troubleshooting
## Tally operation codes
If the tally feature is enabled the following (if default settings are used) will occur:
- ```First color on power up``` is the version
- ```Orange at power up``` the device is connecting to required devices
- ```Turquoise color``` the device is in AP mode

## Tally Error Codes
If the tally feature is enabled the following (if default settings are used) will occur:
- ```Fast flashing blue 20 times before reboot``` Wifi failed to connect or was lost
- ```Fast flashing blue 20 times on startup``` Bluetooth failed to connect
- ```Fast flashing red 20 times on startup``` Tally feature failed to connect
- ```Fast flashing yellow 20 times on startup``` A camera ID was not set

# Special Thanks
This project is made possible by the following
- [SchoolPost](https://github.com/schoolpost/) for [BlueMagic32](https://github.com/schoolpost/BlueMagic32) which provides the bluetooth functionality of this project
- [SKAARHOJ](https://www.skaarhoj.com/) for the research and listing of the commands to interface with the ATEM