/*
  This sketch configures SoftAP and sets the AT firmware to remember it.
  You can start the remembered AP with WiFi.beginAP() without parameters.

  created in Jul 2019 for WiFiEspAT library
  by Juraj Andrassy https://github.com/jandrassy

*/
#include <WiFiEspAT.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
const char ssid[] = SECRET_SSID;    // your network SSID (name)
const char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

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

  WiFi.setPersistent(); // set the following settings as persistent

  WiFi.endAP(); // to disable default automatic start of persistent AP at startup

//  use this lines for custom SoftAP IP. it determines the IP of stations too
//  IPAddress ip(192, 168, 2, 1);
//  WiFi.configureAP(ip);

  Serial.println();
  Serial.print("Start AP with SSID: ");
  Serial.println(ssid);

//  int status = WiFi.beginAP(ssid); // for AP open without passphrase/encryption
  int status = WiFi.beginAP(ssid, pass);

  if (status == WL_AP_LISTENING) {
    Serial.println();
    delay(1000); // startup of AP
    Serial.println("AP started");
    printApStatus();
  } else {
    Serial.println();
    Serial.println("AP failed to start.");
  }

}

void loop() {
}

void printApStatus() {

  char ssid[33];
  WiFi.apSSID(ssid);
  Serial.print("AP SSID: ");
  Serial.println(ssid);

  Serial.print("AP can handle ");
  Serial.print(WiFi.apMaxConnections());
  Serial.println(" stations.");

  if (WiFi.apEncryptionType() == ENC_TYPE_NONE) {
    Serial.println("AP is open (no encryption)");
  } else {
    Serial.print("Encryption of AP is ");
    switch (WiFi.apEncryptionType()) {
      case ENC_TYPE_TKIP:
        Serial.println("WPA.");
        break;
      case ENC_TYPE_CCMP:
        Serial.println("WPA2.");
        break;
    }
    char pass[65];
    WiFi.apPassphrase(pass);
    Serial.print("AP passphrase: ");
    Serial.println(pass);
  }

  uint8_t mac[6];
  WiFi.apMacAddress(mac);
  Serial.print("AP MAC: ");
  printMacAddress(mac);

  IPAddress ip = WiFi.apIP();
  Serial.print("AP IP Address: ");
  Serial.println(ip);
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

