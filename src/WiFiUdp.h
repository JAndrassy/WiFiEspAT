/*
  This file is part of the WiFiEspAT library for Arduino
  https://github.com/jandrassy/WiFiEspAT
  Copyright 2019, 2024 Juraj Andrassy

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
#include "WiFiEspAtSharedBuffStreamPtr.h"
#include "utility/EspAtDrvTypes.h"

class WiFiUDP : public UDP
#ifdef WIFIESPAT1
, protected EspAtDrvUdpDataCallback
#endif
{
public:

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

  virtual uint8_t begin(uint16_t port);

#ifndef WIFIESPAT1 // AT2
  virtual uint8_t beginMulticast(IPAddress ip, uint16_t port);

  // WiFiEspAT AT2 special functions for receive
  size_t availableForParse();
  size_t parsePacket(uint8_t* buffer, size_t bufferSize, IPAddress& remoteIP, uint16_t& remotePort);
#endif

  virtual void stop();

  // Listening for UDP packets
  virtual int parsePacket();
  virtual int available();
  virtual int read();
  virtual int read(uint8_t* buffer, size_t len);
  virtual int read(char* buffer, size_t len) {return read((uint8_t*) buffer, len);}
  virtual int peek();

#ifdef WIFIESPAT1
  virtual IPAddress remoteIP();
  virtual uint16_t remotePort();

protected:
  virtual uint8_t readRxData(Stream* serial, size_t len); // EspAtDrvUdpDataCallback implementation

#else
  virtual IPAddress remoteIP() {return senderIP;}
  virtual uint16_t remotePort() {return senderPort;}

#endif

private:
  uint8_t linkId = WIFIESPAT_NO_LINK;
  bool listening = false;
  WiFiEspAtSharedBuffStreamPtr txStream;
  WiFiEspAtSharedBuffStreamPtr rxStream;
  char strIP[16]; // to hold the string version of IP for beginPacket(ip, port);

#ifndef WIFIESPAT1 //AT2
  IPAddress senderIP;
  uint16_t senderPort;

  uint8_t begin(const char* ip, uint16_t port);
#endif
};

#endif

