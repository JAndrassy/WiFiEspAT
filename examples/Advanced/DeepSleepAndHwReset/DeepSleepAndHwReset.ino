/*
 This example demonstrates hardware reset option
 of the WiFiEspAT library and the deepSleep command.
 Hw reset is required to wakeup the esp from deep sleep.

 Circuit: additionally to RX, TX and ground connection,
 wire the reset pin of the esp8266 to selected pin of your Arduino.

 created in August 2019 for WiFiEspAT library
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

const char* server = "arduino.cc";

const byte ESP_RESET_PIN = 12;
const long SLEEP_INTERVAL = 30000;

WiFiClient client;

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1, ESP_RESET_PIN);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
}

void loop() {

  Serial.println("Waiting for connection to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();
  Serial.println("Connected to WiFi network.");

  doSomething();

  Serial.println();
  Serial.println("putting esp8266 to deep sleep...");
  WiFi.deepSleep();

  delay(SLEEP_INTERVAL); // here the main MCU could be set to sleep too

  WiFi.reset(ESP_RESET_PIN); // wake-up the esp from deep slepp

  Serial.println();
  Serial.println("wakeup");
}

void doSomething() {
  Serial.println();
  Serial.println("Starting connection to server...");
  if (client.connect(server, 80)) {
    Serial.println("connected to server");

    client.println("GET /asciilogo.txt HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    client.flush();
    while (client.connected()) {
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }
    }
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
  }
}

