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

#ifndef _ESPATDRV_TYPES_H_
#define _ESPATDRV_TYPES_H_

#include <stddef.h>

const uint8_t WIFIESPAT_LINKS_COUNT = 5;
const uint8_t WIFIESPAT_NO_LINK = 255;

class EspAtDrvClass;

enum struct EspAtDrvError {
  NO_ERROR,
  NOT_INITIALIZED,
  AT_NOT_RESPONDIG,
  AT_ERROR,
  NO_AP,
  LINK_ALREADY_CONNECTED,
  LINK_NOT_ACTIVE,
  RECEIVE,
  SEND,
  UDP_BUSY,
  UDP_LARGE,
  UDP_TIMEOUT
};

struct WiFiApData {
   char ssid[33];
   uint8_t bssid[6];
   int8_t rssi;
   uint8_t channel;
   uint8_t enc;
};

class EspAtDrvUdpDataCallback {
protected:

  static const uint8_t OK = 0;
  static const uint8_t BUSY = 1;
  static const uint8_t LARGE = 2;
  static const uint8_t TIMEOUT = 3;

  virtual uint8_t readRxData(Stream* serial, size_t len) = 0;
  friend EspAtDrvClass;
};

#endif
