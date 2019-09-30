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

#include "WiFiEspAtBuffStream.h"
#include "utility/EspAtDrv.h"

WiFiEspAtBuffStream::WiFiEspAtBuffStream(uint8_t* _rxBuffer, size_t _rxBufferSize, uint8_t* _txBuffer, size_t _txBufferSize) {
  rxBuffer = _rxBuffer;
  rxBufferSize = _rxBufferSize;
  txBuffer = _txBuffer;
  txBufferSize = _txBufferSize;
  linkId = NO_LINK;
}

void WiFiEspAtBuffStream::setUdpPort(const char* _udpHost, uint16_t _udpPort) {
  udpHost = _udpHost;
  udpPort = _udpPort;
}

void WiFiEspAtBuffStream::reset() {
  linkId = NO_LINK;
  rxBufferLength = 0;
  rxBufferIndex = 0;
  txBufferLength = 0;
  udpPort = 0;
}

size_t WiFiEspAtBuffStream::write(uint8_t b) {
  txBuffer[txBufferLength++] = b;
  if (txBufferLength == txBufferSize) {
    flush();
    if (getWriteError())
      return 0;
  }
  return 1;
}

size_t WiFiEspAtBuffStream::write(const uint8_t *data, size_t length) {
  if (linkId == NO_LINK) {
    setWriteError();
    return 0;
  }
  if (length == 0)
    return 0;
  if (txBufferLength == 0 && length > txBufferSize) // if internal buffer is empty and provided buffer is large
    return EspAtDrv.sendData(linkId, data, length, udpHost, udpPort); // send it right away

  size_t a = txBufferSize - txBufferLength; // available space in internal buffer
  for (size_t i = 0; i < a && i < length; i++) { // copy data to internal buffer
    txBuffer[txBufferLength++] = data[i];
  }
  int d = length - a; // left to write
  if (d >= 0) { // internal buffer is full
    flush();
  }
  if (d <= 0) // nothing more to write
    return length;
  return a + write(data + a, d); // handle the rest of the provided buffer
}

void WiFiEspAtBuffStream::flush() {
  if (txBufferLength == 0)
    return;
  size_t res = EspAtDrv.sendData(linkId, txBuffer, txBufferLength, udpHost, udpPort);
  setWriteError(res != txBufferLength);
  txBufferLength = 0;
}

int WiFiEspAtBuffStream::availableForWrite() {
  return txBufferSize - txBufferLength;
}

int WiFiEspAtBuffStream::available() {
  if (linkId == NO_LINK)
    return 0;
  size_t a = (rxBufferLength - rxBufferIndex);
  if (a == 0) {
    a = EspAtDrv.availData(linkId);
  }
  if (a == 0) {
    flush(); // maybe sketch is waiting for response without flushing the request
  }
  return a;
}

void WiFiEspAtBuffStream::fillRXbuffer() {
  if (rxBufferIndex < rxBufferLength || !available())
    return;
  rxBufferIndex = 0;
  rxBufferLength = EspAtDrv.recvData(linkId, rxBuffer, rxBufferSize);
}

int WiFiEspAtBuffStream::read() {
  if (!available())
    return -1;
  uint8_t b;
  if (!read(&b, 1))
    return -1;
  return b;
}

int WiFiEspAtBuffStream::read(uint8_t* data, size_t size) {
  if (size == 0 || !available())
    return 0;

  size_t l = rxBufferLength - rxBufferIndex;
  if (l == 0 && size > rxBufferSize) // internal buffer is empty and provided buffer is large
    return EspAtDrv.recvData(linkId, data, size); // fill the large provided buffer directly

  // copy from internal buffer
  fillRXbuffer();
  for (size_t i = 0; i < l && i < size; i++) {
    data[i] = rxBuffer[rxBufferIndex++];
  }
  if (size <= l) // provided buffer was filled
    return size;
  return l + read(data + l, size - l); // handle the rest of provided buffer
}

int WiFiEspAtBuffStream::peek() {
  if (!available())
    return -1;
  fillRXbuffer();
  return rxBuffer[rxBufferIndex];
}
