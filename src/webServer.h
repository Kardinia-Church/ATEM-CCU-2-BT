/**
 * Blackmagic CCU to Bluetooth Adaptor For ESP32
 * By Kardinia Church 2020
 * 
 * Tested on a XC3800 ESP32 development board from Jaycar
 * 
 * 
 * webServer.h
 * 
 * Used to serve the web server configuration tool
**/

//#include <WiFi.h>

String responseHTML = "<html>\n"
"    <head>\n"
"        <title>ESP32 Blackmagic CCU Controller</title>\n"
"    </head>\n"
"    <body>\n"
"        <center>\n"
"            <h1>ESP32 Blackmagic CCU Controller</h1>\n"
"            <h2>Please Login</h2>\n"
"            <form name=\"login\" method=\"get\" action=\"index.html\">\n"
"                <h4>Password</h4>\n"
"                <input name=\"password\" type=\"password\"/>\n"
"                <input type=\"submit\"/>\n"
"            </form>\n"
"        </center>\n"
"    </body>\n"
"</html>";

WiFiServer server(80);

//Setup the server to listen for clients
void setupServer() {
    server.begin();
}

//Continuously wait for clients and serve when required
void serverLoop() {
    WiFiClient client = server.available();

    if(client) {
        String incoming = "";
        while(client.connected()) {
            if(client.available()) {
                char c = client.read();
                incoming += c;
                if(c == '\n') {

                    Serial.println(incoming);

                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();
                    client.print(responseHTML);
                    client.stop();
                }
            }
        }
    }
}