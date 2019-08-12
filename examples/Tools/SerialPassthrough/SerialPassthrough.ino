/*
 SerialPassthrough sketch
 with SoftwareSeral option

 created in August 2019 for WiFiEspAT library
 by Juraj Andrassy https://github.com/jandrassy
 */

//#define SAMD_FLOW_CONTROL

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#elif defined(ARDUINO_ARCH_SAMD) && defined(SAMD_FLOW_CONTROL)
#include "wiring_private.h"
Uart SerialAT(&sercom3, 0, 1, SERCOM_RX_PAD_1, UART_TX_PAD_0, 2, 255);
#define AT_BAUD_RATE 115200
#else
#define SerialAT Serial1
#define AT_BAUD_RATE 115200
#endif

void setup() {

  Serial.begin(115200);
  while (!Serial);

  SerialAT.begin(AT_BAUD_RATE);
#if defined(ARDUINO_ARCH_SAMD) && defined(SAMD_FLOW_CONTROL)
  pinPeripheral(0, PIO_SERCOM);
  pinPeripheral(1, PIO_SERCOM);
#endif
}

void loop() {
  while (Serial.available()) {
    SerialAT.write(Serial.read());
  }
  while (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
}

#if defined(ARDUINO_ARCH_SAMD) && defined(SAMD_FLOW_CONTROL)
void SERCOM3_Handler() {
  SerialAT.IrqHandler();
}
#endif
