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

#include "utility/EspAtDrv.h"
#include "WiFiUdp.h"
#include "WiFiEspAtBuffManager.h"

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port) {
  EspAtDrv.ip2str(ip, strIP);
  return beginPacket(strIP, port);
}

int WiFiUDP::beginPacket(const char *host, uint16_t port) {
  if (txStream) {
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
  txStream = WiFiEspAtBuffManager.getBuffStream(linkId, 0, WIFIESPAT_UDP_TX_BUFFER_SIZE);
  if (!txStream) {
   if (!listening) {
    EspAtDrv.close(linkId);
   }
    return false;
  }
  txStream->setUdpPort(host, port);
  return true;
}

int WiFiUDP::endPacket() {
  if (!txStream)
    return 0;
  flush();
  if (listening) {
    txStream->free();
  } else {
    txStream->close();
  }
  txStream = nullptr;
  return !getWriteError();
}

size_t WiFiUDP::write(uint8_t b) {
  if (!txStream)
    return 0;
  return txStream->write(b);
}

size_t WiFiUDP::write(const uint8_t *data, size_t length) {
  if (!txStream)
    return 0;
  return txStream->write(data, length);
}

void WiFiUDP::flush() {
  if (!txStream)
    return;
  txStream->flush();
}

int WiFiUDP::availableForWrite() {
  if (!txStream)
    return 0;
  return txStream->availableForWrite();
}

size_t WiFiUDP::write(SendCallbackFnc callback) {
  if (!txStream)
    return 0;
  return txStream->write(callback);
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

#else

uint8_t WiFiUDP::begin(const char* ip, uint16_t port) {
  if (linkId != NO_LINK) {
    stop();
  }
  linkId = EspAtDrv.connect("UDP", ip, port, port);
  listening = (linkId != NO_LINK);
  return listening;
}

uint8_t WiFiUDP::begin(uint16_t port) {
  return begin("0.0.0.0", port);
}

uint8_t WiFiUDP::beginMulticast(IPAddress ip, uint16_t port) {
  char s[16];
  EspAtDrv.ip2str(ip, s);
  return begin(s, port);
}

size_t WiFiUDP::availableForParse() {
  if (linkId == NO_LINK)
    return 0;
  return EspAtDrv.availData(linkId);
}

size_t WiFiUDP::parsePacket(uint8_t* buffer, size_t size, IPAddress& remoteIP, uint16_t& remotePort) {
  size_t len = availableForParse();
  if (!len)
    return 0;
  if (len > size) {
    len = size;
  }
  return EspAtDrv.recvDataWithInfo(linkId, buffer, len, remoteIP, remotePort);
}
#endif

void WiFiUDP::stop() {
  if (rxStream) {
    rxStream->free();
    rxStream = nullptr;
  }
  if (txStream) {
    endPacket();
  }
  if (linkId == NO_LINK)
    return;
  EspAtDrv.close(linkId);
  linkId = NO_LINK;
  listening = false;
}

int WiFiUDP::parsePacket() {
  if (linkId == NO_LINK)
    return 0;
#ifdef WIFIESPAT1
  if (rxStream && rxStream->rxBufferIndex > 0) { // clear already read packet
    rxStream->free();
    rxStream = nullptr;
  }
  EspAtDrv.maintain();
#else
  if (rxStream) {
    rxStream->free();
    rxStream = nullptr;
  }
  if (!availableForParse())
    return 0;
  rxStream = WiFiEspAtBuffManager.getBuffStream(NO_LINK, WIFIESPAT_UDP_RX_BUFFER_SIZE, 0);
  if (!rxStream)
    return 0;
  rxStream->rxBufferLength = WiFiUDP::parsePacket(rxStream->rxBuffer, WIFIESPAT_UDP_RX_BUFFER_SIZE, senderIP, senderPort);
#endif
  return available();
}

int WiFiUDP::available() {
  if (!rxStream)
    return 0;
  return rxStream->available();
}

int WiFiUDP::read() {
  if (!available())
    return -1;
  return rxStream->read();
}

int WiFiUDP::read(uint8_t* data, size_t size) {
  if (size == 0 || !available())
    return 0;
  return rxStream->read(data, size);
}

int WiFiUDP::peek() {
  if (!available())
    return -1;
  return rxStream->peek();
}

#ifdef WIFIESPAT1
uint8_t WiFiUDP::readRxData(Stream* serial, size_t len) {
  if (available() > 0) // to avoid overwrite of previous packet
    return BUSY;
  if (len > WIFIESPAT_UDP_RX_BUFFER_SIZE)
    return LARGE;
  rxStream = WiFiEspAtBuffManager.getBuffStream(NO_LINK, WIFIESPAT_UDP_RX_BUFFER_SIZE, 0);
  if (!rxStream)
    return BUSY;
  size_t l = serial->readBytes(rxStream->rxBuffer, len);
  if (l != len) { // timeout
    rxStream->free();
    rxStream = nullptr;
    return TIMEOUT;
  }
  rxStream->rxBufferLength = len;
  return OK;
}
#endif
