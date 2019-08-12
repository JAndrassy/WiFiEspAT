/*
 Chat  Server

 A simple server that echoes any incoming messages to a
 connected client. To use telnet to your device's IP address and type.
 You can see the client's input in the serial monitor as well.

 created 18 Dec 2009
 by David A. Mellis
 modified 31 May 2012
 by Tom Igoe

 modified in Jul 2019 for WiFiEspAT library
 by Juraj Andrassy https://github.com/jandrassy

*/
#include <WiFiEspAT.h>

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

WiFiServer server(2323);
WiFiClient client;

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

  server.begin();

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.println("Connected to WiFi network.");
  Serial.print("To access the server, connect with Telnet client to ");
  Serial.print(ip);
  Serial.println(" 2323\".");
}


void loop() {

  if (!client) {
    client = server.available();
    if (client) {
      Serial.println("We have a new client");
      client.println("Hello, client!");
      client.flush();
    }
  }

  if (client) {
    while (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      // echo the bytes back to the client:
      client.write(thisChar);
      // echo the bytes to the server as well:
      Serial.write(thisChar);
    }
    client.flush();
    if (!client.connected()) {
      Serial.println("Client disconnected.");
    }
  }
}
