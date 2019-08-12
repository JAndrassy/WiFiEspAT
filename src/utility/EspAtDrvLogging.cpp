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

#include "EspAtDrvLogging.h"
#include <Arduino.h> // to include PROGMEM in a compatible way

#if LOG_LEVEL >= LOG_LEVEL_ERROR
const char LOG_ERROR_PREFIX_PROGMEM[] PROGMEM = LOG_ERROR_PREFIX;
#endif
#if LOG_LEVEL >= LOG_LEVEL_WARN
const char LOG_WARN_PREFIX_PROGMEM[] PROGMEM = LOG_WARN_PREFIX;
#endif
#if LOG_LEVEL >= LOG_LEVEL_INFO
const char LOG_INFO_PREFIX_PROGMEM[] PROGMEM = LOG_INFO_PREFIX;
#endif
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
const char LOG_DEBUG_PREFIX_PROGMEM[] PROGMEM = LOG_DEBUG_PREFIX;
#endif
