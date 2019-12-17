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

#ifndef _WIFIESPAT_CONFIG_H_
#define _WIFIESPAT_CONFIG_H_

#ifndef WIFIESPAT_INTERNAL_AP_LIST_SIZE
#if defined(__AVR__) && RAMEND <= 0x8FF
#define WIFIESPAT_INTERNAL_AP_LIST_SIZE 3
#else
#define WIFIESPAT_INTERNAL_AP_LIST_SIZE 6
#endif
#endif

#ifndef WIFIESPAT_CLIENT_RX_BUFFER_SIZE
#if defined(__AVR__) && RAMEND <= 0x8FF
#define WIFIESPAT_CLIENT_RX_BUFFER_SIZE 32
#else
#define WIFIESPAT_CLIENT_RX_BUFFER_SIZE 64
#endif
#endif

#ifndef WIFIESPAT_CLIENT_TX_BUFFER_SIZE
#if defined(__AVR__) && RAMEND <= 0x8FF
#define WIFIESPAT_CLIENT_TX_BUFFER_SIZE 32
#else
#define WIFIESPAT_CLIENT_TX_BUFFER_SIZE 64
#endif
#endif

#if WIFIESPAT_CLIENT_RX_BUFFER_SIZE == 0
#define WIFIESPAT_CLIENT_RX_BUFFER_SIZE 1
#warning WiFiClient RX buffer size must be at least 1
#endif

#ifndef WIFIESPAT_UDP_TX_BUFFER_SIZE
#if defined(__AVR__) && RAMEND <= 0x8FF
#define WIFIESPAT_UDP_TX_BUFFER_SIZE 64
#else
#define WIFIESPAT_UDP_TX_BUFFER_SIZE 256
#endif
#endif

#ifndef WIFIESPAT_UDP_RX_BUFFER_SIZE
#if defined(__AVR__) && RAMEND <= 0x8FF
#define WIFIESPAT_UDP_RX_BUFFER_SIZE 64
#else
#define WIFIESPAT_UDP_RX_BUFFER_SIZE 256
#endif
#endif

#endif
