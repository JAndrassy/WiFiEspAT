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
  uint8_t linkId;
  if (listening) {
    linkId = this->linkId; // AT allows to use the listener's linkId for sending
  } else {
    linkId = EspAtDrv.connect("UDP", host, port);
  }
  if (linkId == NO_LINK)
    return false;
  stream = WiFiEspAtBuffManager.getBuffStream(linkId, 0, 0, WIFIESPAT_UDP_TX_BUFFER_SIZE);
  if (!stream) {
   if (!listening) {
    EspAtDrv.close(linkId);
   }
    return false;
  }
  stream->setUdpPort(host, port);
  return true;
}

int WiFiUdpSender::endPacket() {
  if (!stream)
    return 0;
  flush();
  if (listening) {
    stream->free();
  } else {
    stream->close();
  }
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

#ifdef WIFIESPAT1
IPAddress WiFiUDP::remoteIP() {
  IPAddress ip;
  uint16_t port = 0;
  uint16_t lport = 0;
  if (linkId != NO_LINK) {
    EspAtDrv.remoteParamsQuery(linkId, ip, port, lport);
  }
  return ip;
}

uint16_t WiFiUDP::remotePort() {
  IPAddress ip;
  uint16_t port = 0;
  uint16_t lport = 0;
  if (linkId != NO_LINK) {
    EspAtDrv.remoteParamsQuery(linkId, ip, port, lport);
  }
  return port;
}

#endif


#ifndef WIFIESPAT1 // AT2 WiFiEspAtUDP functions
uint8_t WiFiUdpSender::begin(const char* ip, uint16_t port) {
  if (linkId != NO_LINK) {
    stop();
  }
  linkId = EspAtDrv.connect("UDP", ip, port, port);
  listening = (linkId != NO_LINK);
  return listening;
}

uint8_t WiFiUdpSender::begin(uint16_t port) {
  return begin("0.0.0.0", port);
}

uint8_t WiFiUdpSender::beginMulticast(IPAddress ip, uint16_t port) {
  char s[16];
  EspAtDrv.ip2str(ip, s);
  return begin(s, port);
}

size_t WiFiUdpSender::availableForParse() {
  if (linkId == NO_LINK)
    return 0;
  return EspAtDrv.availData(linkId);
}

size_t WiFiUdpSender::parsePacket(uint8_t* buffer, size_t size, IPAddress& remoteIP, uint16_t& remotePort) {
  size_t len = availableForParse();
  if (!len)
    return 0;
  if (len > size) {
    len = size;
  }
  return EspAtDrv.recvDataWithInfo(linkId, buffer, len, remoteIP, remotePort);
}
#endif

void WiFiUdpSender::stop() {
  if (linkId == NO_LINK)
    return;
  if (stream) {
    endPacket();
  }
  EspAtDrv.close(linkId);
  linkId = NO_LINK;
  listening = false;
}

#ifdef WIFIESPAT1
uint8_t WiFiUDP::begin(uint16_t port) {
  if (linkId != NO_LINK) {
    stop();
  }
  linkId = EspAtDrv.connect("UDP", "0.0.0.0", port, this, port);
  listening = (linkId != NO_LINK);
  return listening;
}
#endif

int WiFiUDP::parsePacket() {
  if (linkId == NO_LINK)
    return 0;
#ifdef WIFIESPAT1
  if (rxBufferLength > 0 && rxBufferIndex > 0) { // clear already read packet
    rxBufferLength = 0;
    rxBufferIndex = 0;
  }
  EspAtDrv.maintain();
#else
  rxBufferIndex = 0;
  rxBufferLength = WiFiUdpSender::parsePacket(rxBuffer, sizeof(rxBuffer), senderIP, senderPort);
#endif  
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

#ifdef WIFIESPAT1
uint8_t WiFiUDP::readRxData(Stream* serial, size_t len) {
  if (available() > 0) // to avoid overwrite of previous packet
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
#endif
