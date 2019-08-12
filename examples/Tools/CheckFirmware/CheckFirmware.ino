/*
  This sketch checks the version of the AT firmware in attached esp8266.

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

  char ver[10];
  int major = 0;
  int minor = 0;
  if (WiFi.firmwareVersion(ver)) {
    Serial.print("AT firmware version ");
    Serial.println(ver);
    char* tok = strtok(ver, ".");
    major = atoi(tok);
    tok = strtok(NULL, ".");
    minor = atoi(tok);
    if (major != 1 || minor < 7) {
      if (major == 2 && minor == 0) {
        Serial.println("AT firmware version 2.0 doesn't support passive receive mode and can't be used with the WiFiEspAt library");
      } else {
        Serial.println("WiWiEspAT library requires at least version 1.7.0 of AT firmware (but not 2.0)");
      }
    } else {
      Serial.println("AT firmware is OK for the WiFiEspAT library.");
    }
  } else {
    Serial.println("Error getting AT firmware version");
  }


}

void loop() {
}
