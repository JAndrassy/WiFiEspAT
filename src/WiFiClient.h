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

#ifndef _WIFICLIENT_H_
#define _WIFICLIENT_H_

#include <Client.h>
#include "WiFiEspAtBuffStream.h"
#include "WiFiEspAtConfig.h"

enum WiFiTcpState {
  CLOSED      = 0,
  LISTEN      = 1,
  ESTABLISHED = 4
};

class WiFiServer;

class WiFiClient : public Client {

  friend WiFiServer;
  WiFiClient(uint8_t linkId);

public:
  WiFiClient();

  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual void stop();
          void abort();

  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual void flush();

  virtual int available();
  virtual int read();
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek();

  virtual operator bool();
  virtual uint8_t connected();
  uint8_t status();

  virtual IPAddress remoteIP();
  virtual uint16_t remotePort();

  using Print::write;

  uint8_t getLinkId() { return linkId; }

private:
  uint8_t linkId;

  byte rxBuffer[WIFIESPAT_CLIENT_RX_BUFFER_SIZE];
  byte txBuffer[WIFIESPAT_CLIENT_TX_BUFFER_SIZE];

  WiFiEspAtBuffStream stream;

};

#endif
