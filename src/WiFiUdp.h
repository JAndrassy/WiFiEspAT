/*
  This file is part of the WiFiEspAT library for Arduino
  https://github.com/jandrassy/WiFiEspAT
  Copyright 2019 Juraj Andrassy

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _WIFIUDP_H_
#define _WIFIUDP_H_

#include <Udp.h>
#include "WiFiEspAtConfig.h"
#include "WiFiEspAtBuffStream.h"
#include "utility/EspAtDrvTypes.h"

class WiFiUdpSender : public UDP {
public:

  WiFiUdpSender();

  // Sending UDP packets
  virtual int beginPacket(IPAddress ip, uint16_t port);
  virtual int beginPacket(const char *host, uint16_t port);
  virtual int endPacket();

  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buffer, size_t size);
  virtual void flush();
  virtual int availableForWrite();

  size_t write(SendCallbackFnc callback);

  using Print::write;

  virtual IPAddress remoteIP();
  virtual uint16_t remotePort();

  // Listening for UDP packets - not implemented here
  virtual uint8_t begin(uint16_t) {return 0;}
  virtual void stop() {}

  virtual int parsePacket() {return 0;}

  virtual int available() {return 0;}
  virtual int read() {return -1;}
  virtual int read(uint8_t* buffer, size_t len) { (void) buffer; (void) len; return 0;}
  virtual int read(char* buffer, size_t len) { (void) buffer; (void) len; return 0;}
  virtual int peek() { return -1;}

  uint8_t getLinkId() { return linkId; }

protected:

  uint8_t linkId;
  WiFiEspAtBuffStream stream;

private:
  byte txBuffer[WIFIESPAT_UDP_TX_BUFFER_SIZE];

  char strIP[16]; // to hold the string version of IP for beginPacket(ip, port);

};

class WiFiUDP : public WiFiUdpSender, protected EspAtDrvUdpDataCallback {
public:

  WiFiUDP();

  virtual uint8_t begin(uint16_t);
  virtual void stop();

  // Listening for UDP packets
  virtual int parsePacket();
  virtual int available();
  virtual int read();
  virtual int read(uint8_t* buffer, size_t len);
  virtual int read(char* buffer, size_t len) {return read((uint8_t*) buffer, len);}
  virtual int peek();

  // Sending UDP packets
  virtual int beginPacket(const char *host, uint16_t port);
  using WiFiUdpSender::beginPacket; // for version with IPAddress
  virtual int endPacket();
  using Print::write;

protected:
  virtual uint8_t readRxData(Stream* serial, size_t len); // EspAtDrvUdpDataCallback implementation

private:

  bool listening = false;

  byte rxBuffer[WIFIESPAT_UDP_RX_BUFFER_SIZE];
  size_t rxBufferIndex = 0;
  size_t rxBufferLength = 0;
};

#endif

