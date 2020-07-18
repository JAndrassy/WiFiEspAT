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

#ifndef _ESP_AT_BUFF_STREAM_H_
#define _ESP_AT_BUFF_STREAM_H_

#include <Stream.h>
#include <IPAddress.h>
#include "utility/EspAtDrvTypes.h"

class WiFiEspAtBuffStream {

public:

  void setUdpPort(const char* udpHost, uint16_t udpPort);
  bool connected();
  void free();
  void close(bool abort = false);

  uint8_t getLinkId() {return linkId;}

  size_t write(uint8_t);
  size_t write(const uint8_t *buf, size_t size);
  void flush();
  int availableForWrite();

  size_t write(Stream& file);
  size_t write(SendCallbackFnc callback);

  int8_t getWriteError() {return writeError;}

  int available();
  int read();
  int read(uint8_t *buf, size_t size);
  int peek();

private:
  friend class WiFiEspAtBuffManagerClass;

  void fillRXbuffer();
  void setWriteError(int8_t err = -1) {writeError = err;}

  bool assigned = false;
  uint8_t linkId = WIFIESPAT_NO_LINK;
  const char* udpHost = nullptr;
  uint16_t port = 0;

  int8_t writeError = 0;

  uint8_t* rxBuffer = nullptr;
  size_t rxBufferSize = 0;
  size_t rxBufferIndex = 0;
  size_t rxBufferLength = 0;

  uint8_t* txBuffer = nullptr;
  size_t txBufferSize = 0;
  size_t txBufferLength = 0;

};

#endif
