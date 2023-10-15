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

#ifndef _WIFISERVER_H_
#define _WIFISERVER_H_

#include "WiFiClient.h"

class WiFiServer {

public:
  WiFiServer(uint16_t port = 80);
  void begin() { begin(1, 60); }
  void begin(uint8_t maxConnCount, uint16_t serverTimeout);
  void begin(uint16_t port) { begin(port, 1, 60); }
  void begin(uint16_t port, uint8_t maxConnCount, uint16_t serverTimeout);
  void beginSSL() { beginSSL(false, 1, 60); }
  void beginSSL(bool ca, uint8_t maxConnCount, uint16_t serverTimeout);
  void beginSSL(uint16_t port) { beginSSL(port, false, 1, 60); }
  void beginSSL(uint16_t port, bool ca, uint8_t maxConnCount, uint16_t serverTimeout);
  void end();
  uint8_t status();
  WiFiClient available(bool accept = false);
  WiFiClient accept() {return available(true);}
  virtual operator bool();

protected:
  size_t writeToAllClients(const uint8_t *buf, size_t size);
  void flushAllClients();

private:
  uint16_t port;
  uint8_t state;
};

class WiFiServerPrint : public WiFiServer, public Print {

public:
  WiFiServerPrint(uint16_t port = 80) : WiFiServer(port) {}

  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual void flush();

  using Print::write;

};

#endif
