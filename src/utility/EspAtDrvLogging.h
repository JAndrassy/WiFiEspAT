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

#ifndef _WIFIESPAT_DEBUG_H_
#define _WIFIESPAT_DEBUG_H_

#include <Arduino.h>

#define FSH_P const __FlashStringHelper*

#define LOG_LEVEL_SILENT 0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4

#ifndef WIFIESPAT_LOG_LEVEL
#define WIFIESPAT_LOG_LEVEL LOG_LEVEL_SILENT
#endif

#ifdef ARDUINO_SAM_ZERO // M0
#define LOG_OUTPUT SerialUSB
#else
#define LOG_OUTPUT Serial
#endif

#define LOG_ERROR_PREFIX "esp ERROR: "
#if WIFIESPAT_LOG_LEVEL >= LOG_LEVEL_ERROR
extern const char LOG_ERROR_PREFIX_PROGMEM[];
#define LOG_ERROR_PRINT_PREFIX() LOG_OUTPUT.print((FSH_P) LOG_ERROR_PREFIX_PROGMEM)
#define LOG_ERROR_PRINT(msg) LOG_OUTPUT.print(msg)
#define LOG_ERROR_PRINTLN(msg) LOG_OUTPUT.println(msg)
#else
#define LOG_ERROR_PRINT_PREFIX()
#define LOG_ERROR_PRINT(msg)
#define LOG_ERROR_PRINTLN(msg)
#endif

#define LOG_WARN_PREFIX "esp WARN: "
#if WIFIESPAT_LOG_LEVEL >= LOG_LEVEL_WARN
extern const char LOG_WARN_PREFIX_PROGMEM[];
#define LOG_WARN_PRINT_PREFIX() LOG_OUTPUT.print((FSH_P) LOG_WARN_PREFIX_PROGMEM)
#define LOG_WARN_PRINT(msg) LOG_OUTPUT.print(msg)
#define LOG_WARN_PRINTLN(msg) LOG_OUTPUT.println(msg)
#else
#define LOG_WARN_PRINT_PREFIX()
#define LOG_WARN_PRINT(msg)
#define LOG_WARN_PRINTLN(msg)
#endif

#define LOG_INFO_PREFIX "esp INFO: "
#if WIFIESPAT_LOG_LEVEL >= LOG_LEVEL_INFO
extern const char LOG_INFO_PREFIX_PROGMEM[];
#define LOG_INFO_PRINT_PREFIX() LOG_OUTPUT.print((FSH_P) LOG_INFO_PREFIX_PROGMEM)
#define LOG_INFO_PRINT(msg) LOG_OUTPUT.print(msg)
#define LOG_INFO_PRINTLN(msg) LOG_OUTPUT.println(msg)
#else
#define LOG_INFO_PRINT_PREFIX()
#define LOG_INFO_PRINT(msg)
#define LOG_INFO_PRINTLN(msg)
#endif

#define LOG_DEBUG_PREFIX "esp> "
#if WIFIESPAT_LOG_LEVEL >= LOG_LEVEL_DEBUG
extern const char LOG_DEBUG_PREFIX_PROGMEM[];
#define LOG_DEBUG_PRINT_PREFIX() LOG_OUTPUT.print((FSH_P) LOG_DEBUG_PREFIX_PROGMEM)
#define LOG_DEBUG_PRINT(msg) LOG_OUTPUT.print(msg)
#define LOG_DEBUG_PRINTLN(msg) LOG_OUTPUT.println(msg)
#else
#define LOG_DEBUG_PRINT_PREFIX()
#define LOG_DEBUG_PRINT(msg)
#define LOG_DEBUG_PRINTLN(msg)
#endif

#endif
