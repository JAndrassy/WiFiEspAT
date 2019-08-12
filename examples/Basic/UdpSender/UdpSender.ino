/*
  WiFi UDP Send String

 This sketch sends an UDP packet using an Arduino board with WiFiEspAT library.

 created in Jul 2019 for WiFiEspAT library
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

WiFiUdpSender Udp;
IPAddress reciverIP(192, 168, 1, 108);
unsigned int receiverPort = 9876;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  // check for the WiFi module:
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
  Serial.println("Connected to WiFi network.");
}

void loop() {

  Udp.beginPacket(reciverIP, receiverPort);
  Udp.print("Arduino millis ");
  Udp.print(millis());
  Udp.endPacket();

  delay(5000);
}
