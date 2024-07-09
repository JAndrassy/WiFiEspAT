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

#ifndef WIFIESPAT_SERVER_MAX_CLIENTS
#define WIFIESPAT_SERVER_MAX_CLIENTS 1
#endif

#ifndef WIFIESPAT_SERVER_CLIENT_TIMEOUT
#define WIFIESPAT_SERVER_CLIENT_TIMEOUT 60  // seconds
#endif

class WiFiServer {

public:
  WiFiServer(uint16_t port = 80);
  void begin() { begin(WIFIESPAT_SERVER_MAX_CLIENTS, WIFIESPAT_SERVER_CLIENT_TIMEOUT); }
  void begin(uint8_t maxConnCount, uint16_t serverTimeout);
  void begin(uint16_t port) { begin(port, WIFIESPAT_SERVER_MAX_CLIENTS, WIFIESPAT_SERVER_CLIENT_TIMEOUT); }
  void begin(uint16_t port, uint8_t maxConnCount, uint16_t serverTimeout);
  void beginSSL() { beginSSL(false, WIFIESPAT_SERVER_MAX_CLIENTS, WIFIESPAT_SERVER_CLIENT_TIMEOUT); }
  void beginSSL(bool ca, uint8_t maxConnCount, uint16_t serverTimeout);
  void beginSSL(uint16_t port) { beginSSL(port, false, WIFIESPAT_SERVER_MAX_CLIENTS, WIFIESPAT_SERVER_CLIENT_TIMEOUT); }
  void beginSSL(uint16_t port, bool ca, uint8_t maxConnCount, uint16_t serverTimeout);
  void end();
  uint8_t status();
  WiFiClient available() __attribute__((deprecated("Use accept().")));
  WiFiClient accept();
  virtual operator bool();

private:
  uint16_t port;
  uint8_t state;
};

#endif
