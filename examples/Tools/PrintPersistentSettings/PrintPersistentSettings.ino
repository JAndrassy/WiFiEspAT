/*
  This sketch prints the configuration remembered in the esp8266.

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

  int status = WiFi.beginAP();

  if (status == WL_AP_LISTENING) {
    Serial.println();
    printApStatus();
    WiFi.endAP();
  } else {
    Serial.println();
    Serial.println("AP failed to start.");
  }

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  Serial.println();
  Serial.println("Waiting for connection to WiFi");
  for (int i = 0; i < 6; i++) {
    if (WiFi.status() == WL_CONNECTED)
      break;
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  printWifiStatus();
}

void loop() {
}

void printWifiStatus() {

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("Station MAC: ");
  printMacAddress(mac);

  char ssid[33];
  WiFi.SSID(ssid);
  if (strlen(ssid) == 0) {
    Serial.println("Station is not connected");
    return;
  }
  Serial.print("SSID: ");
  Serial.println(ssid);

  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.print(rssi);
  Serial.println(" dBm");

  bool dhcp = WiFi.dhcpIsEnabled();
  Serial.print("DHCP: ");
  Serial.println(dhcp ? "enabled" : "disabled");

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  IPAddress gw = WiFi.gatewayIP();
  Serial.print("gateway IP Address: ");
  Serial.println(gw);

  IPAddress mask = WiFi.subnetMask();
  Serial.print("subnet IP mask: ");
  Serial.println(mask);

  Serial.print("DNS server: ");
  IPAddress dns1 = WiFi.dnsServer1();
  if (dns1 == INADDR_NONE) {
    Serial.println("not set");
  } else {
    dns1.printTo(Serial);
    Serial.println();
    IPAddress dns2 = WiFi.dnsServer2();
    if (dns2 != INADDR_NONE) {
      Serial.print("DNS server2: ");
      dns2.printTo(Serial);
      Serial.println();
    }
  }
}

void printApStatus() {

  uint8_t mac[6];
  WiFi.apMacAddress(mac);
  Serial.print("AP MAC: ");
  printMacAddress(mac);

  char ssid[33];
  WiFi.apSSID(ssid);
  Serial.print("AP SSID: ");
  Serial.print(ssid);
  if (WiFi.apIsHidden()) {
    Serial.print(" (is hidden)");
  }
  Serial.println();

  Serial.print("AP max stations ");
  Serial.println(WiFi.apMaxConnections());

  Serial.print("AP encryption: ");
  switch (WiFi.apEncryptionType()) {
    case ENC_TYPE_NONE:
      Serial.println("open (no encryption)");
      break;
    case ENC_TYPE_TKIP:
      Serial.println("WPA");
      break;
    case ENC_TYPE_CCMP:
      Serial.println("WPA2");
      break;
  }

  if (WiFi.apEncryptionType() != ENC_TYPE_NONE) {
    char pass[65];
    WiFi.apPassphrase(pass);
    Serial.print("AP passphrase: ");
    Serial.println(pass);
  }

  IPAddress ip = WiFi.apIP();
  Serial.print("AP IP Address: ");
  Serial.println(ip);

  IPAddress gw = WiFi.apGatewayIP();
  Serial.print("AP gateway IP Address: ");
  Serial.println(gw);

  IPAddress mask = WiFi.apSubnetMask();
  Serial.print("AP subnet mask: ");
  Serial.println(mask);

  Serial.print("AP DHCP: ");
  Serial.println(WiFi.apDhcpIsEnabled() ? "enabled" : "disabled");
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

