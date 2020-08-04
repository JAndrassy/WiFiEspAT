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

#include "utility/EspAtDrv.h"
#include "WiFiServer.h"

WiFiServer::WiFiServer(uint16_t _port) {
  port = _port;
  state = CLOSED;
}

void WiFiServer::begin(uint8_t maxConnCount, uint8_t serverTimeout) {
  state = EspAtDrv.serverBegin(port, maxConnCount, serverTimeout) ? LISTEN : CLOSED;
}

void WiFiServer::beginSSL(bool ca, uint8_t maxConnCount, uint8_t serverTimeout) {
  state = EspAtDrv.serverBegin(port, maxConnCount, serverTimeout, true, ca) ? LISTEN : CLOSED;
}

void WiFiServer::end() {
  if (EspAtDrv.serverEnd()) {
    state = CLOSED;
  }
}

uint8_t WiFiServer::status() {
  return state;
}

WiFiClient WiFiServer::available(bool accept) {
  if (state != CLOSED) {
    uint8_t linkId = EspAtDrv.clientLinkId(accept);
    if (linkId != NO_LINK)
      return WiFiClient(linkId, port);
  }
  return WiFiClient();
}

WiFiServer::operator bool() {
  return (state != CLOSED);
}

