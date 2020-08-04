/*
  WiFi SSL Web Server only for AT 2 on ESP32

 A SSL web server that shows the value of the analog input pins.

 created 13 July 2010
 by dlf (Metodo2 srl)
 modified 31 May 2012
 by Tom Igoe
 modified in August 2020 for WiFiEspAT library
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

WiFiServer server(443);

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
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

  server.beginSSL();

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.println("Connected to WiFi network.");
  Serial.print("To access the server, enter \"https://");
  Serial.print(ip);
  Serial.println("/\" in web browser.");
}

void loop() {

  WiFiClient client = server.available();
  if (client) { // true only for a connected client with data available
    IPAddress ip = client.remoteIP();
    Serial.print("new client ");
    Serial.println(ip);

    size_t size = client.available(); // for AT SSL server all at once arrive and must be read at once
    char buff[size]; // size is around 400
    size_t l = client.read((uint8_t*) buff, size);
    if (l == 0) {
      Serial.print("error reading request");
    } else {
      buff[l] = 0; // true count is always less then reported available count for SSL on AT2
      Serial.print(buff);

      // send a standard http response header
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");  // the connection will be closed after completion of the response
      client.println("Refresh: 5");  // refresh the page automatically every 5 sec
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");

      // output the value of analog input pins
      for (int analogChannel = 0; analogChannel < 4; analogChannel++) {
        int sensorReading = analogRead(analogChannel);
        client.print("analog input ");
        client.print(analogChannel);
        client.print(" is ");
        client.print(sensorReading);
        client.println("<br />");
      }
      client.println("</html>");
    }

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

