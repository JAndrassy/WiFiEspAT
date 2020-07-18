/*
  This file is part of the WiFiEspAT library for Arduino
  https://github.com/jandrassy/WiFiEspAT
  Copyright 2020 Juraj Andrassy

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

#ifndef _ESP_AT_BUFF_MAN_H_
#define _ESP_AT_BUFF_MAN_H_

#include "WiFiEspAtBuffStream.h"

class WiFiEspAtBuffManagerClass {
public:

  WiFiEspAtBuffManagerClass();

  WiFiEspAtBuffStream* getBuffStream(uint8_t linkId, uint16_t serverPort, size_t rxBufferSize, size_t txBufferSize);

  void freeUnused();

private:

  WiFiEspAtBuffStream* pool[WIFIESPAT_LINKS_COUNT];

};

extern WiFiEspAtBuffManagerClass WiFiEspAtBuffManager;

#endif
