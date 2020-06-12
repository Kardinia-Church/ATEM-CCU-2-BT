#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Arduino.h"
#include <WiFi.h>
#include "prefHandler.h"

class WebServer {
    private: 
        PreferencesHandler *prefHandler;
        WiFiServer *server = new WiFiServer(80);
        String responseHTML = "<html>\n"
            "    <head>\n"
            "        <title>ATEM2BT Camera Controller</title>\n"
            "    </head>\n"
            "    <body>\n"
            "        <center>\n"
            "            <h1>ATEM2BT Camera Controller</h1>\n"
            "            <h2>Please Login</h2>\n"
            "            "
            "            <form name=\"login\" method=\"get\" action=\"index.html\">\n"
            "                <h4>Password</h4>\n"
            "                <input name=\"password\" type=\"password\"/>\n"
            "                <input type=\"submit\"/>\n"
            "            </form>\n"
            "        </center>\n"
            "    </body>\n"
            "</html>";
    public:
        void start(PreferencesHandler *preferencesHandler) {
            prefHandler = preferencesHandler;
            server->begin();
        }

        void loop() {
            WiFiClient client = server->available();

            if(client) {
                String incoming = "";
                while(client.connected()) {
                    if(client.available()) {
                        char c = client.read();
                        incoming += c;
                        if(c == '\n') {
                            // if(incoming.length() != 0) {
                            //     Serial.println(incoming);
                            //     if (incoming.indexOf("GET /") >= 0) {
                            //     }
                            // }
                            


                            












                            Serial.println(incoming);

                            // client.println("HTTP/1.1 200 OK");
                            // client.println("Content-type:text/html");
                            // client.println();
                            // client.print(responseHTML);
                            client.stop();
                        }
                    }
                }
            }
        }
};

#endif