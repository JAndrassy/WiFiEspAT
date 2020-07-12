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
#include "WiFiClient.h"

WiFiClient::WiFiClient() : stream(rxBuffer, sizeof(rxBuffer), txBuffer, sizeof(txBuffer)) {
  linkId = NO_LINK;
}

WiFiClient::WiFiClient(uint8_t _linkId) : stream(rxBuffer, sizeof(rxBuffer), txBuffer, sizeof(txBuffer)) {
  linkId = _linkId;
  stream.setLinkId(linkId);
}

int WiFiClient::connect(bool ssl, const char* host, uint16_t port) {
  if (linkId != NO_LINK) {
    stop();
  }
  linkId = EspAtDrv.connect(ssl ? "SSL" : "TCP", host, port);
  stream.setLinkId(linkId);
  return (linkId != NO_LINK);
}

int WiFiClient::connect(bool ssl, IPAddress ip, uint16_t port) {
  char s[16];
  EspAtDrv.ip2str(ip, s);
  return connect(ssl, s, port);
}

int WiFiClient::connect(const char* host, uint16_t port) {
  return connect(false, host, port);
}

int WiFiClient::connect(IPAddress ip, uint16_t port) {
  return connect(false, ip, port);
}

int WiFiClient::connectSSL(const char* host, uint16_t port) {
  return connect(true, host, port);
}

int WiFiClient::connectSSL(IPAddress ip, uint16_t port) {
  return connect(true, ip, port);
}

void WiFiClient::stop() {
  if (linkId == NO_LINK)
    return;
  flush();
  stream.reset();
  EspAtDrv.close(linkId);
  linkId = NO_LINK;
}

void WiFiClient::abort() {
  if (linkId == NO_LINK)
    return;
  stream.reset();
  EspAtDrv.close(linkId, true);
  linkId = NO_LINK;
}

size_t WiFiClient::write(uint8_t b) {
  return stream.write(b);
}

size_t WiFiClient::write(const uint8_t *data, size_t length) {
  return stream.write(data, length);
}

void WiFiClient::flush() {
  stream.flush();
}

size_t WiFiClient::write(Stream& file) {
  return stream.write(file);
}

size_t WiFiClient::write(SendCallbackFnc callback) {
  return stream.write(callback);
}

int WiFiClient::available() {
  return stream.available();
}

int WiFiClient::read() {
  return stream.read();
}

int WiFiClient::read(uint8_t* data, size_t size) {
  return stream.read(data, size);
}

int WiFiClient::peek() {
  return stream.peek();
}

WiFiClient::operator bool() {
  return connected();
}

uint8_t WiFiClient::connected() {
  return (status() == ESTABLISHED || available()); // Arduino WiFi library examples expect connected true while data are available
}

uint8_t WiFiClient::status() {
  if (linkId == NO_LINK)
    return CLOSED;
  if (EspAtDrv.connected(linkId))
    return ESTABLISHED;
  linkId = NO_LINK;
  return CLOSED;
}

IPAddress WiFiClient::remoteIP() {
  IPAddress ip;
  uint16_t port = 0;
  if (linkId != NO_LINK) {
    EspAtDrv.remoteParamsQuery(linkId, ip, port);
  }
  return ip;
}

uint16_t WiFiClient::remotePort() {
  IPAddress ip;
  uint16_t port = 0;
  if (linkId != NO_LINK) {
    EspAtDrv.remoteParamsQuery(linkId, ip, port);
  }
  return port;
}
