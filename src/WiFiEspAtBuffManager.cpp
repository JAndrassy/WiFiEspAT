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

#include "WiFiEspAtBuffManager.h"
#include "utility/EspAtDrvLogging.h"

WiFiEspAtBuffManagerClass::WiFiEspAtBuffManagerClass() {
  for (int i = 0; i < WIFIESPAT_LINKS_COUNT; i++) {
    pool[i] = nullptr;
  }
}

WiFiEspAtBuffStream* WiFiEspAtBuffManagerClass::getBuffStream(uint8_t linkId, uint16_t serverPort, size_t rxBufferSize, size_t txBufferSize) {

  if (linkId == WIFIESPAT_NO_LINK)
    return nullptr;

  int freePos = -1;

  for (int i = 0; i < WIFIESPAT_LINKS_COUNT; i++) {
    if (pool[i] == nullptr) {
      freePos = i;
      break;
    }
    if (pool[i]->linkId == linkId) {
      if (serverPort && serverPort == pool[i]->port) {
        LOG_INFO_PRINT_PREFIX();
        LOG_INFO_PRINT(F("BuffManager returned buff.stream at "));
        LOG_INFO_PRINT(i);
        LOG_INFO_PRINT(F(" for server at port "));
        LOG_INFO_PRINT(serverPort);
        LOG_INFO_PRINT(F(" and client's linkId "));
        LOG_INFO_PRINTLN(linkId);
        return pool[i];
      } else {
        pool[i]->free();
        LOG_WARN_PRINT_PREFIX();
        LOG_WARN_PRINT(F("BuffManager cleared linkId "));
        LOG_WARN_PRINT(linkId);
        LOG_WARN_PRINT(F(" at "));
        LOG_WARN_PRINTLN(i);
      }
    }
    if (pool[i]->assigned)
      continue;
    if (pool[i]->rxBufferSize == rxBufferSize && pool[i]->txBufferSize == txBufferSize) {
      pool[i]->linkId = linkId;
      pool[i]->port = serverPort;
      pool[i]->assigned = true;
      LOG_INFO_PRINT_PREFIX();
      LOG_INFO_PRINT(F("BuffManager returned buff.stream at "));
      LOG_INFO_PRINT(i);
      if (serverPort) {
        LOG_INFO_PRINT(F(" for server at port "));
        LOG_INFO_PRINT(serverPort);
      }
      LOG_INFO_PRINT(F(" for linkId "));
      LOG_INFO_PRINTLN(linkId);
      return pool[i];
    }
  }
  if (freePos == -1) {
    LOG_WARN_PRINT_PREFIX();
    LOG_WARN_PRINTLN(F("getBuffStream no free position"));
    return nullptr;
  }
  WiFiEspAtBuffStream *res = new WiFiEspAtBuffStream();
  if (rxBufferSize) {
    res->rxBuffer = new uint8_t[rxBufferSize];
  }
  if (txBufferSize) {
    res->txBuffer = new uint8_t[txBufferSize];
  }
  res->rxBufferSize = rxBufferSize;
  res->txBufferSize = txBufferSize;
  res->linkId = linkId;
  res->port = serverPort;
  res->assigned = true;
  pool[freePos] = res;
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("BuffManager new buff.stream at "));
  LOG_INFO_PRINT(freePos);
  if (serverPort) {
    LOG_INFO_PRINT(F(" for server at port "));
    LOG_INFO_PRINT(serverPort);
  }
  LOG_INFO_PRINT(F(" for linkId "));
  LOG_INFO_PRINT(linkId);
  LOG_INFO_PRINT(F(" rx "));
  LOG_INFO_PRINT(rxBufferSize);
  LOG_INFO_PRINT(F(" tx "));
  LOG_INFO_PRINTLN(txBufferSize);
  return res;
}

void WiFiEspAtBuffManagerClass::freeUnused() {
  for (int i = 0; i < WIFIESPAT_LINKS_COUNT; i++) {
    if (pool[i] == nullptr)
      break;
    if (pool[i]->linkId == WIFIESPAT_NO_LINK) {
      LOG_INFO_PRINT_PREFIX();
      LOG_INFO_PRINT(F("BuffManager free tx "));
      LOG_INFO_PRINTLN(pool[i]->txBufferSize);
      if (pool[i]->rxBuffer != nullptr) {
        delete pool[i]->rxBuffer;
      }
      if (pool[i]->txBuffer != nullptr) {
        delete pool[i]->txBuffer;
      }
      delete pool[i];
      pool[i] = nullptr;
    }
  }
  int i = 0;
  for (; i < WIFIESPAT_LINKS_COUNT && pool[i] != nullptr; i++);
  int j = i;
  for (; i < WIFIESPAT_LINKS_COUNT; i++) {
    if (pool[i] != nullptr) {
      pool[j++] = pool[i];
      pool[i] = nullptr;
    }
  }
}


WiFiEspAtBuffManagerClass WiFiEspAtBuffManager;
