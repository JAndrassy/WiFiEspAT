/*
 The purpose of this sketch is to switch the default baud rate
 of the AT firmware to SoftwareSerial friendly 9600 baud.
 SoftwareSerial has problems to receive at 115200 baud, but is
 able to send a short string at 115200 baud. So we send
 at 115200 baud over SoftwareSerial the AT command to change
 the baud rate to 9600 baud.

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

#define OLD_BAUD_RATE 115200
#define NEW_BAUD_RATE 9600

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(OLD_BAUD_RATE);

  Serial.println("Sending baud rate change...");
  Serial1.print("AT+UART_DEF=");
  Serial1.print(NEW_BAUD_RATE);
  Serial1.println(",8,1,0,0");
  delay(100);
  // we can't expect a readable answer over SoftwareSerial at 115200

  Serial1.begin(NEW_BAUD_RATE);
  WiFi.init(Serial1);
  const char* ver = WiFi.firmwareVersion();
  if (ver[0] != 0) {
    Serial.print("Baud rate ");
    Serial.print(NEW_BAUD_RATE);
    Serial.println(" baud is working.");
    Serial.print("Firmware version is ");
    Serial.println(ver);
  } else {
    Serial.print("Error communicating at ");
    Serial.print(NEW_BAUD_RATE);
    Serial.println(" baud.");
  }
}

void loop() {
}
