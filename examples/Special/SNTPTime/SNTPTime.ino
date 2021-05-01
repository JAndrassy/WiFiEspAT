/*
  SNTP Time with customized AT firmware
  It doesn't work with standard Espressif AT firmware

 created in Jul 2019 for WiFiEspAT library
 by Juraj Andrassy https://github.com/jandrassy
 */

#include <WiFiEspAT.h>
#include <TimeLib.h> // in LibraryManager as "Time"

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

const int8_t TIME_ZONE = 2; // UTC + 2

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

  WiFi.sntp("us.pool.ntp.org");

  Serial.println("Waiting for SNTP");
  while (WiFi.getTime() < SECS_YR_2000) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  setTime(WiFi.getTime() + (SECS_PER_HOUR * TIME_ZONE));

}

void loop() {

  char buff[20];
  sprintf(buff, "%02d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  Serial.println(buff);

  delay(1000);
}

