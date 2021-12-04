/*
  Configuration AP example
  The example shows a very simple Configuration Access Point.

  created in August 2019 for WiFiEspAT library
  by Juraj Andrassy https://github.com/jandrassy

*/
#include <WiFiEspAT.h>

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

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

  WiFi.disconnect(); // stop the persistent connection to test the Configuration AP

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  Serial.println();
  Serial.println("Waiting for connection to WiFi");
  for (int i = 0; i < 5; i++) {
    if (WiFi.status() == WL_CONNECTED)
      break;
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Could not connect to WiFi. Starting configuration AP...");
    configAP();
  } else {
    Serial.println("You're connected to the network");
  }
}

void loop() {
}

void configAP() {

  WiFiServer server(80);

  WiFi.beginAP(); // starts the default AP (factory default or setup as persistent)

  char ssid[33];
  WiFi.apSSID(ssid);
  Serial.print("Connect your computer to the WiFi network ");
  Serial.print(ssid);
  Serial.println();
  IPAddress ip = WiFi.apIP();
  Serial.print("and enter http://");
  Serial.print(ip);
  Serial.println(" in a Web browser");

  server.begin();

  while (true) {

    WiFiClient client = server.available();
    if (client && client.available()) { // if !available yet, we return to this client in next loop
      char line[64];
      int l = client.readBytesUntil('\n', line, sizeof(line));
      line[l] = 0;
      client.find((char*) "\r\n\r\n");

      if (strncmp_P(line, PSTR("POST"), strlen("POST")) == 0) {
        l = client.readBytes(line, sizeof(line));
        line[l] = 0;

        // parse the parameters sent by the html form
        const char* delims = "=&";
        strtok(line, delims);
        const char* ssid = strtok(NULL, delims);
        strtok(NULL, delims);
        const char* pass = strtok(NULL, delims);

        // send a response before attemting to connect to the WiFi network
        // because it will reset the SoftAP and disconnect the client station
        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Connection: close"));
        client.println(F("Refresh: 10"));
        client.println();
        client.println(F("<html><body><h3>Configuration AP</h3><br>connecting...</body></html>"));
        client.flush();
        delay(1000);
        client.stop();

        Serial.println();
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        // WiFi.setPersistent(); // to make a successful connection persistent
        WiFi.begin(ssid, pass);

        // configuration continues with the next request

      } else {

        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Connection: close"));
        client.println();
        client.println(F("<html><body><h3>Configuration AP</h3><br>"));

        int status = WiFi.status();
        if (status == WL_CONNECTED) {
          client.println(F("Connection successful. Ending AP."));
        } else {
          client.println(F("<form action='/' method='POST'>WiFi connection failed. Enter valid parameters, please.<br><br>"));
          client.println(F("SSID:<br><input type='text' name='i'><br>"));
          client.println(F("Password:<br><input type='password' name='p'><br><br>"));
          client.println(F("<input type='submit' value='Submit'></form>"));
        }
        client.println(F("</body></html>"));
        client.stop();

        if (status == WL_CONNECTED) {
          delay(1000); // to let the AT firmware finish the communication
          Serial.println("Connection successful. Ending AP.");
          server.end();
          WiFi.endAP(true);
          return;
        }
      }
    }
  }
}
