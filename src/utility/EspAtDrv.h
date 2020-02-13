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

#ifndef _ESP_AT_DRV_H_
#define _ESP_AT_DRV_H_

#include <Arduino.h>
#include <IPAddress.h>
#include "utility/EspAtDrvTypes.h"

const uint8_t LINKS_COUNT = WIFIESPAT_LINKS_COUNT;
const uint8_t NO_LINK = WIFIESPAT_NO_LINK;

const uint8_t LINK_CONNECTED = (1 << 0);
const uint8_t LINK_CLOSING = (1 << 1);
const uint8_t LINK_IS_INCOMING = (1 << 2);
const uint8_t LINK_IS_ACCEPTED = (1 << 3);
const uint8_t LINK_IS_UDP_LISTNER = (1 << 4);

struct LinkInfo {
  uint8_t flags = 0;
  size_t available = 0;

  EspAtDrvUdpDataCallback* udpDataCallback;

  bool isConnected() { return flags & LINK_CONNECTED;}
  bool isClosing() { return flags & LINK_CLOSING;}
  bool isIncoming() { return flags & LINK_IS_INCOMING;}
  bool isAccepted() { return flags & LINK_IS_ACCEPTED;}
  bool isUdpListener() { return flags & LINK_IS_UDP_LISTNER;}
};

class EspAtDrvClass {
public:

  bool init(Stream* serial, int8_t resetPin = -1);

  bool reset(int8_t resetPin = -1);
  void maintain();
  EspAtDrvError getLastErrorCode() {return lastErrorCode;}
  bool firmwareVersion(char* buff);

  int staStatus();

  uint8_t listAP(WiFiApData apData[], uint8_t size); // returns count of filled records

  bool staStaticIp(const IPAddress& local_ip, const IPAddress& gateway, const IPAddress& subnet, bool persistent = false);
  bool staDNS(const IPAddress& dns1, const IPAddress& dns2, bool persistent = false);
  bool staMacQuery(uint8_t* mac);
  bool staIpQuery(IPAddress& ip, IPAddress& gwip, IPAddress& mask);
  bool staDnsQuery(IPAddress& dns1, IPAddress& dns2);

  bool joinAP(const char* ssid, const char* password, const uint8_t* bssid, bool persistent = false);
  bool quitAP();
  bool staAutoConnect(bool autoConnect);
  bool apQuery(char* ssid, uint8_t* bssid, uint8_t& channel, int32_t& rssi);

  bool softApIp(const IPAddress& local_ip, const IPAddress& gateway, const IPAddress& subnet, bool persistent = false);
  bool softApMacQuery(uint8_t* mac);
  bool softApIpQuery(IPAddress& ip, IPAddress& gwip, IPAddress& mask);

  bool beginSoftAP(const char *ssid = nullptr, const char* passphrase = nullptr, uint8_t channel = 1,
      uint8_t encoding = 4, uint8_t maxConnections = 0, bool hidden = false, bool persistent = false);
  bool endSoftAP(bool persistent = false);
  bool softApQuery(char* ssid, char* passphrase, uint8_t& channel, uint8_t& encoding, uint8_t& maxConnections, bool& hidden);

  bool serverBegin(uint16_t port, uint8_t maxConnCount = 1, uint8_t serverTimeout = 60);
  bool serverEnd();
  uint8_t clientLinkId(bool accept = false);

  uint8_t connect(const char* type, const char* host, uint16_t port, //
      EspAtDrvUdpDataCallback* udpDataCallback = nullptr, uint16_t udpLocalPort = 0);
  bool close(uint8_t linkId, bool abort = false);
  bool remoteParamsQuery(uint8_t linkId, IPAddress& remoteIP, uint16_t& remoteParamsQuery);

  bool connected(uint8_t linkId);
  size_t availData(uint8_t linkId);

  size_t recvData(uint8_t linkId, uint8_t buff[], size_t buffSize);
  size_t sendData(uint8_t linkId, const uint8_t buff[], size_t dataLength, const char* udpHost, uint16_t udpPort);
  size_t sendData(uint8_t linkId, Stream& file, const char* udpHost, uint16_t udpPort);
  size_t sendData(uint8_t linkId, SendCallbackFnc callback, const char* udpHost, uint16_t udpPort);

  bool setHostname(const char* hostname);
  bool hostnameQuery(char* hostname);
  bool dhcpStateQuery(bool& staDHCP, bool& softApDHCP); // they have nothing in common, but use the same command
  bool mDNS(const char* hostname, const char* serverName, uint16_t serverPort);
  bool resolve(const char* hostname, IPAddress& result);
  bool sntpCfg(int8_t timezone, const char* server1, const char* server2);
  unsigned long sntpTime();
  bool ping(const char* hostname);

  bool sleepMode(EspAtSleepMode mode);
  bool deepSleep();

  void ip2str(const IPAddress& ip, char* s);

private:
  Stream* serial;
  Print* cmd; // debug wrapper or serial
  char buffer[64];
  uint8_t wifiMode = 0;
  uint8_t wifiModeDef = 0;
  LinkInfo linkInfo[LINKS_COUNT];
  EspAtDrvError lastErrorCode = EspAtDrvError::NOT_INITIALIZED;
  unsigned long lastSyncMillis;

  uint8_t freeLinkId();

  bool readRX(PGM_P expected, bool bufferData = true, bool listItem = false);
  bool readOK();
  bool sendCommand(PGM_P expected = nullptr, bool bufferData = true, bool listItem = false);
  bool simpleCommand(PGM_P cmd);

  bool setWifiMode(uint8_t mode, bool persistent = false);
  bool syncLinkInfo();
  bool recvLenQuery();
  bool checkLinks();
};

extern EspAtDrvClass EspAtDrv;

#endif
