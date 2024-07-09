/*
 Pager  Server

 A simple server that echoes any incoming messages to all
 connected clients. Connect two or more telnet sessions
 to see how server.available() and server.print() works.

 For version 2 of the WiFiEspAT library this example uses
 ArduinoWiFiServer from the NetApiHelpers library
 because the WiFiEspAT library doesn't directly support
 server.available() and print-to-all-clients anymore.

 created in September 2020 for WiFiEspAT library
 modified in July 2024 for version 2 of the WiFiEspAT
 by Juraj Andrassy https://github.com/jandrassy

*/
#include <WiFiEspAT.h>
#include <NetApiHelpers.h>

#define SERVER_MAX_MONITORED_CLIENTS 3 // applied in ArduinoWiFiServer.h
#include <ArduinoWiFiServer.h>

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

const int CLIENT_CONN_TIMEOUT = 3600; // seconds. 1 hour

ArduinoWiFiServer server(2323);

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  Serial.println("Waiting for connection to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  server.begin(SERVER_MAX_MONITORED_CLIENTS, CLIENT_CONN_TIMEOUT);

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.println("Connected to WiFi network.");
  Serial.print("To access the server, connect with Telnet client to ");
  Serial.print(ip);
  Serial.println(" 2323");
}

void loop() {

  WiFiClient client = server.available(); // returns first client which has data to read or a 'false' client
  if (client) { // client is true only if it is connected and has data to read
    String s = client.readStringUntil('\n'); // read the message incoming from one of the clients
    s.trim(); // trim eventual \r
    Serial.println(s); // print the message to Serial Monitor
    client.print("echo: "); // this is only for the sending client
    server.println(s); // send the message to all connected clients
    server.flush(); // flush the buffers
  }
}
