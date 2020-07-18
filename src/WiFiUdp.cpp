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
#include "WiFiUdp.h"
#include "WiFiEspAtBuffManager.h"

int WiFiUdpSender::beginPacket(IPAddress ip, uint16_t port) {
  EspAtDrv.ip2str(ip, strIP);
  return beginPacket(strIP, port);
}

int WiFiUdpSender::beginPacket(const char *host, uint16_t port) {
  if (stream) {
    endPacket();
  }
  uint8_t linkId = EspAtDrv.connect("UDP", host, port);
  if (linkId == NO_LINK)
    return false;
  stream = WiFiEspAtBuffManager.getBuffStream(linkId, 0, 0, WIFIESPAT_UDP_TX_BUFFER_SIZE);
  if (!stream) {
    EspAtDrv.close(linkId);
    return false;
  }
  stream->setUdpPort(host, port);
  return true;
}

int WiFiUdpSender::endPacket() {
  if (!stream)
    return 0;
  flush();
  stream->close();
  stream = nullptr;
  return !getWriteError();
}

size_t WiFiUdpSender::write(uint8_t b) {
  if (!stream)
    return 0;
  return stream->write(b);
}

size_t WiFiUdpSender::write(const uint8_t *data, size_t length) {
  if (!stream)
    return 0;
  return stream->write(data, length);
}

void WiFiUdpSender::flush() {
  if (!stream)
    return;
  stream->flush();
}

int WiFiUdpSender::availableForWrite() {
  if (!stream)
    return 0;
  return stream->availableForWrite();
}

size_t WiFiUdpSender::write(SendCallbackFnc callback) {
  if (!stream)
    return 0;
  return stream->write(callback);
}

IPAddress WiFiUDP::remoteIP() {
  IPAddress ip;
  uint16_t port = 0;
  if (linkId != NO_LINK) {
    EspAtDrv.remoteParamsQuery(linkId, ip, port);
  }
  return ip;
}

uint16_t WiFiUDP::remotePort() {
  IPAddress ip;
  uint16_t port = 0;
  if (linkId != NO_LINK) {
    EspAtDrv.remoteParamsQuery(linkId, ip, port);
  }
  return port;
}

WiFiUDP::WiFiUDP() {
}


uint8_t WiFiUDP::begin(uint16_t port) {
  if (linkId != NO_LINK) {
    stop();
  }
  linkId = EspAtDrv.connect("UDP", "0.0.0.0", port, this, port);
  listening = (linkId != NO_LINK);
  return listening;
}

void WiFiUDP::stop() {
  if (linkId == NO_LINK)
    return;
  if (stream) {
    endPacket();
  }
  EspAtDrv.close(linkId);
  linkId = NO_LINK;
  listening = false;
}

int WiFiUDP::parsePacket() {
  if (linkId == NO_LINK)
    return 0;
  rxBufferLength = 0;
  rxBufferIndex = 0;
  EspAtDrv.maintain();
  return available();
}

int WiFiUDP::available() {
  return (rxBufferLength - rxBufferIndex);
}

int WiFiUDP::read() {
  if (!available())
    return -1;
  return rxBuffer[rxBufferIndex++];
}

int WiFiUDP::read(uint8_t* data, size_t size) {
  if (size == 0 || !available())
    return 0;
  size_t l = rxBufferLength - rxBufferIndex;
  if (l > size) {
    l = size;
  }
  memcpy(data, rxBuffer + rxBufferIndex, l);
  rxBufferIndex += l;
  return l;
}

int WiFiUDP::peek() {
  if (!available())
    return -1;
  return rxBuffer[rxBufferIndex];
}

uint8_t WiFiUDP::readRxData(Stream* serial, size_t len) {
  if (rxBufferLength) // to avoid overwrite of previous packet
    return BUSY;
  if (len > sizeof(rxBuffer))
    return LARGE;
  size_t l = serial->readBytes(rxBuffer, len);
  if (l != len) // timeout
    return TIMEOUT;
  rxBufferLength = len;
  rxBufferIndex = 0;
  return OK;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port) {
  if (!listening)
    return WiFiUdpSender::beginPacket(host, port);
  if (stream) {
    endPacket();
  }
  // AT allows to use the listener's linkId
  stream = WiFiEspAtBuffManager.getBuffStream(linkId, 0, 0, WIFIESPAT_UDP_TX_BUFFER_SIZE);
  if (!stream)
    return false;
  stream->setUdpPort(host, port);
  return true;
}

int WiFiUDP::endPacket() {
  if (!listening)
    return WiFiUdpSender::endPacket();
  flush();
  stream->free();
  stream = nullptr;
  return !getWriteError();
}

