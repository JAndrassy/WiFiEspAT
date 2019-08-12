/*
 This example connects to a Wifi network if it is accessible.
 Then it prints the  MAC address of the Wifi module,
 the IP address obtained, and other network details.

  based on the ConnectWithWPA.ino
  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe

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

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
const char ssid[] = SECRET_SSID;    // your network SSID (name)
const char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

void setup() {

  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // disconnect and delete previous connection
  WiFi.disconnect();

}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println();
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);

    int status = WiFi.begin(ssid, pass);
    Serial.println();

    if (status != WL_CONNECTED) {
      Serial.println("Failed to connect to AP");
    } else {
      Serial.println("You're connected to the network");
      printWifiData();
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    printCurrentNet();
  }
  delay(10000);
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

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
