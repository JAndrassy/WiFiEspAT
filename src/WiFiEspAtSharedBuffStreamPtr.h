/*
  This file is part of the WiFiEspAT library for Arduino
  https://github.com/jandrassy/WiFiEspAT
  Copyright 2024 Juraj Andrassy

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

#ifndef _ESP_AT_BUFF_PTR_H_
#define _ESP_AT_BUFF_PTR_H_

#include "WiFiEspAtBuffStream.h"

class WiFiEspAtSharedBuffStreamPtr {
public:

  WiFiEspAtSharedBuffStreamPtr() {}

  WiFiEspAtSharedBuffStreamPtr(WiFiEspAtBuffStream* ptr) {
    if (ptr != nullptr) {
      this->ptr = ptr;
      serialId = ptr->serialId;
      increaseRefCount();
    }
  }

  WiFiEspAtSharedBuffStreamPtr(WiFiEspAtSharedBuffStreamPtr& other) {
    ptr = other.ptr;
    serialId = other.serialId;
    increaseRefCount();
  }

  WiFiEspAtSharedBuffStreamPtr(WiFiEspAtSharedBuffStreamPtr&& other) {
    ptr = other.ptr;
    serialId = other.serialId;
    other.ptr = nullptr;
  }

  ~WiFiEspAtSharedBuffStreamPtr() {
    decreaseRefCount();
  }

  WiFiEspAtSharedBuffStreamPtr& operator=(const WiFiEspAtSharedBuffStreamPtr& other) {
    checkValid();
    if (ptr != other.ptr && serialId != other.serialId) {
      decreaseRefCount();
      ptr = other.ptr;
      serialId = other.serialId;
      increaseRefCount();
    }
    return *this;
  }

  WiFiEspAtSharedBuffStreamPtr& operator=(WiFiEspAtSharedBuffStreamPtr&& other) {
    checkValid();
    if (ptr != other.ptr && serialId != other.serialId) {
      decreaseRefCount();
      ptr = other.ptr;
      serialId = other.serialId;
      other.ptr = nullptr;
    }
    return *this;
  }

  WiFiEspAtBuffStream* operator->() const {
    return ptr;
  }

  explicit operator bool() const {
    return (ptr != nullptr && serialId == ptr->serialId);
  }

private:
  WiFiEspAtBuffStream* ptr = nullptr;
  uint8_t serialId = 0;

  bool checkValid() {
    if (ptr != nullptr && serialId != ptr->serialId) {
      ptr = nullptr;
    }
    return (ptr != nullptr);
  }

  void increaseRefCount() {
    if (checkValid()) {
      ptr->refCount++;
    }
  }
  void decreaseRefCount() {
    if (checkValid()) {
      ptr->refCount--;
      if (ptr->refCount == 0) {
        ptr->close();
        ptr = nullptr;
      }
    }
  }
};

#endif
