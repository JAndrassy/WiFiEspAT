/*
  WiFi UDP Receive String with provided buffer

 This sketch wait an UDP packet on localPort using the WiFi module.
 When a packet is received an Acknowledge packet is sent to the client on port remotePort

 This example requires WiFiEspAT library with ESP AT 2 firmware
 and uses WiFiEspAT specific UDP receiving functions.

 created 30 December 2012
 by dlf (Metodo2 srl)
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

unsigned int localPort = 2390;      // local port to listen on

char reply[] = "acknowledged";       // a string to send back

WiFiUDP udp;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  // check for the WiFi module:
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
  Serial.println("Connected to wifi");
  printWifiStatus();
  

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  udp.begin(localPort);
}

void loop() {

  // if there's data available, read a packet
  size_t packetSize = udp.availableForParse();
  if (packetSize) {
    char packetBuffer[packetSize + 1]; // +1 for the terminating 0
    IPAddress remoteIp;
    uint16_t remotePort = 0;
    size_t len = udp.parsePacket((uint8_t*) packetBuffer, packetSize, remoteIp, remotePort);
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(remotePort);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    Serial.println("Contents:");
    Serial.println(packetBuffer);

    // send a reply, to the IP address and port that sent us the packet we received
    udp.beginPacket(remoteIp, remotePort);
    udp.write(reply);
    udp.endPacket();
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

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




