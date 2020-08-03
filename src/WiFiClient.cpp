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
#include "WiFiEspAtBuffManager.h"

WiFiClient::WiFiClient() {
}

WiFiClient::WiFiClient(uint8_t linkId, uint16_t serverPort) {
  stream = WiFiEspAtBuffManager.getBuffStream(linkId, serverPort, WIFIESPAT_CLIENT_RX_BUFFER_SIZE, WIFIESPAT_CLIENT_TX_BUFFER_SIZE);
  if (!stream) {
    EspAtDrv.close(linkId);
  }
}

int WiFiClient::connect(bool ssl, const char* host, uint16_t port) {
  if (stream) {
    stop();
  }
  uint8_t linkId = EspAtDrv.connect(ssl ? "SSL" : "TCP", host, port);
  if (linkId == NO_LINK)
    return false;
  stream = WiFiEspAtBuffManager.getBuffStream(linkId, 0, WIFIESPAT_CLIENT_RX_BUFFER_SIZE, WIFIESPAT_CLIENT_TX_BUFFER_SIZE);
  if (!stream) {
    EspAtDrv.close(linkId);
    return false;
  }
  return true;
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
  if (!stream)
    return;
  flush();
  stream->close();
  stream = nullptr;
}

void WiFiClient::abort() {
  if (!stream)
    return;
  stream->close(true);
  stream = nullptr;
}

size_t WiFiClient::write(uint8_t b) {
  if (!stream)
    return 0;
  return stream->write(b);
}

size_t WiFiClient::write(const uint8_t *data, size_t length) {
  if (!stream)
    return 0;
  return stream->write(data, length);
}

void WiFiClient::flush() {
  if (!stream)
    return;
  stream->flush();
}

size_t WiFiClient::write(Stream& file) {
  if (!stream)
    return 0;
  return stream->write(file);
}

size_t WiFiClient::write(SendCallbackFnc callback) {
  if (!stream)
    return 0;
  return stream->write(callback);
}

int WiFiClient::available() {
  if (!stream)
    return 0;
  return stream->available();
}

int WiFiClient::read() {
  if (!stream)
    return -1;
  return stream->read();
}

int WiFiClient::read(uint8_t* data, size_t size) {
  if (!stream)
    return 0;
  return stream->read(data, size);
}

int WiFiClient::peek() {
  if (!stream)
    return -1;
  return stream->peek();
}

WiFiClient::operator bool() {
  return (stream != nullptr);
}

uint8_t WiFiClient::connected() {
  if (!stream)
    return false;
  if (stream->connected() || available()) // Arduino WiFi library examples expect connected true while data are available
    return true;
  // link is closed and all data from stream are read
  stream->free();
  stream = nullptr;
  return false;
}

uint8_t WiFiClient::status() {
  if (!stream)
    return CLOSED;
  if (stream->connected())
    return ESTABLISHED;
  return CLOSED;
}

IPAddress WiFiClient::remoteIP() {
  IPAddress ip;
  uint16_t port = 0;
  if (stream && stream->getLinkId() != NO_LINK) {
    EspAtDrv.remoteParamsQuery(stream->getLinkId(), ip, port);
  }
  return ip;
}

uint16_t WiFiClient::remotePort() {
  IPAddress ip;
  uint16_t port = 0;
  if (stream && stream->getLinkId() != NO_LINK) {
    EspAtDrv.remoteParamsQuery(stream->getLinkId(), ip, port);
  }
  return port;
}
