/*
  This sketch connects to a WiFi network and sets the AT firmware
  to remember this WiFi network and auto-connect to it at startup.

  based on the ConnectWithWPA.ino
  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe

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

  WiFi.endAP(true); // to disable default automatic start of persistent AP at startup

  WiFi.setPersistent(); // set the following WiFi connection as persistent

//  uncomment this lines for persistent static IP. set addresses valid for your network
//  IPAddress ip(192, 168, 1, 9);
//  IPAddress gw(192, 168, 1, 1);
//  IPAddress nm(255, 255, 255, 0);
//  WiFi.config(ip, gw, gw, nm);

  Serial.println();
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

//  use following lines if you want to connect with bssid
//  const byte bssid[] = {0x8A, 0x2F, 0xC3, 0xE9, 0x25, 0xC0};
//  int status = WiFi.begin(ssid, pass, bssid);

  int status = WiFi.begin(ssid, pass);

  if (status == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to WiFi network.");
    printWifiStatus();
  } else {
    WiFi.disconnect(); // remove the WiFi connection
    Serial.println();
    Serial.println("Connection to WiFi network failed.");
  }
}

void loop() {
}

void printWifiStatus() {

  // print the SSID of the network you're attached to:
  char ssid[33];
  WiFi.SSID(ssid);
  Serial.print("SSID: ");
  Serial.println(ssid);

  // print the BSSID of the network you're attached to:
  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  printMacAddress(mac);

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
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

