/*
  Telnet Console

 This sketch expects esp8266 on Serial and doesn't use Serial for debug output.
 It uses the built in LED to indicate WiFi connection error
 and Telnet server for debug prints.
 Make sure that logging is set to SILENT in EspAtDrvLogging.h

 Configure a static IP address or write down the DHCP assigned IP address
 printed by the SetupWiFiConnection sketch.

 After the LED stops blinking start Telnet client application on IP address
 and port 2323. On Windows or Linux command prompt you would write
 for example
 telnet 192.168.1.104 2323

 only one client can connect at the time

 created in August 2019 for WiFiEspAT library
 by Juraj Andrassy https://github.com/jandrassy
 */

#include <WiFiEspAT.h>

#define AT_BAUD_RATE 115200

WiFiServer telnetServer(2323);
WiFiClient telneClient;
bool newClient = false;

void setup() {

  Serial.begin(AT_BAUD_RATE);
  WiFi.init(Serial);

  pinMode(LED_BUILTIN, OUTPUT);

  if (WiFi.status() == WL_NO_MODULE) {
    while (true) {
      blink(100);
    }
  }

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  while (WiFi.status() != WL_CONNECTED) {
    blink(500);
  }

  telnetServer.begin();

}

void loop() {

  if (!telneClient) {  // client is not connected
    telneClient = telnetServer.accept(); // returns active or 'empty' client
  }

  if (telneClient.connected()) {
    if (!newClient) {
      newClient = true;
      telneClient.println();
      telneClient.println("Welcome");
      telneClient.println("You can send C to close the connection.");
      delay(5000);
    }
    telneClient.println();
    for (int analogChannel = 0; analogChannel < 4; analogChannel++) {
      int sensorReading = analogRead(analogChannel);
      telneClient.print("analog input A");
      telneClient.print(analogChannel);
      telneClient.print(" is ");
      telneClient.println(sensorReading);
    }
    telneClient.flush();
    if (telneClient.read() == 'C') { // close command from client
      telneClient.stop();
      newClient = false;
    }
  } else if (telneClient) { // stop disconnected client
    telneClient.stop();
    newClient = false;
  }
  delay(5000);
}

void blink(int interval) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(interval);
  digitalWrite(LED_BUILTIN, LOW);
  delay(interval);
}
