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

#include "WiFi.h"
#include "utility/EspAtDrv.h"

// this statics are removed by compiler if not used
char WiFiClass::fwVersion[10] = {0};
char WiFiClass::ssid[33] = {0};
char WiFiClass::name[33] = {0}; // hostname
WiFiApData WiFiClass::apDataInternal[WIFIESPAT_INTERNAL_AP_LIST_SIZE];

bool WiFiClass::init(Stream& serial, int8_t resetPin) {
  return init(&serial, resetPin);
}

bool WiFiClass::init(Stream* serial, int8_t resetPin) {
  bool ok = EspAtDrv.init(serial, resetPin);
  state = ok ? WL_IDLE_STATUS : WL_NO_MODULE;
  return ok;
}

void WiFiClass::setPersistent(bool _persistent) {
  persistent = _persistent;
}

uint8_t WiFiClass::status() {
  if (state == WL_NO_MODULE)
    return state;
  int res = EspAtDrv.staStatus();
  switch (res) {
    case -1:
      switch (EspAtDrv.getLastErrorCode()) {
        case EspAtDrvError::NOT_INITIALIZED:
        case EspAtDrvError::AT_NOT_RESPONDIG:
          state = WL_NO_MODULE;
          break;
        default: // some temporary error?
          break; // no change
      }
      break;
    case 2:
    case 3:
    case 4:
      state = WL_CONNECTED;
      break;
    case 0: // inactive
    case 1: // idle
    case 5: // STA disconnected
      switch (state) {
        case WL_CONNECT_FAILED:
          break; // no change
        case WL_CONNECTED:
          state = WL_CONNECTION_LOST;
          break;
        default:
          state = WL_DISCONNECTED;
      }
  }
  return state;
}

bool WiFiClass::setAutoConnect(bool autoConnect) {
  return EspAtDrv.staAutoConnect(autoConnect);
}

int WiFiClass::begin(const char* ssid, const char* passphrase, const uint8_t* bssid) {
  bool ok = EspAtDrv.joinAP(ssid, passphrase, bssid, persistent);
  state = ok ? WL_CONNECTED : WL_CONNECT_FAILED;
  return state;
}

int WiFiClass::disconnect() {
  if (EspAtDrv.quitAP()) {
    state = WL_DISCONNECTED;
  }
  return state;
}

bool WiFiClass::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet) {
  return EspAtDrv.staStaticIp(local_ip, gateway, subnet, persistent) && setDNS(dns_server);
}

bool WiFiClass::setDNS(IPAddress dns_server1, IPAddress dns_server2) {
  return EspAtDrv.staDNS(dns_server1, dns_server2, persistent);
}

bool WiFiClass::setHostname(const char* name) {
  return EspAtDrv.setHostname(name);
}

const char* WiFiClass::hostname(char* buffer) {
  if (!EspAtDrv.hostnameQuery(buffer)) {
    buffer[0] = 0;
  }
  return buffer;
}

uint8_t* WiFiClass::macAddress(uint8_t* mac) {
  if (!EspAtDrv.staMacQuery(mac))
    return nullptr;
  return mac;
}

IPAddress WiFiClass::localIP() {
  IPAddress ip;
  IPAddress gw;
  IPAddress mask;
  EspAtDrv.staIpQuery(ip, gw, mask);
  return ip;
}

IPAddress WiFiClass::gatewayIP() {
  IPAddress ip;
  IPAddress gw;
  IPAddress mask;
  EspAtDrv.staIpQuery(ip, gw, mask);
  return gw;
}

IPAddress WiFiClass::subnetMask() {
  IPAddress ip;
  IPAddress gw;
  IPAddress mask;
  EspAtDrv.staIpQuery(ip, gw, mask);
  return mask;
}

IPAddress WiFiClass::dnsServer1() {
  IPAddress dns1;
  IPAddress dns2;
  EspAtDrv.staDnsQuery(dns1, dns2);
  return dns1;
}

IPAddress WiFiClass::dnsServer2() {
  IPAddress dns1;
  IPAddress dns2;
  EspAtDrv.staDnsQuery(dns1, dns2);
  return dns2;
}

bool WiFiClass::dhcpIsEnabled() {
  bool sta;
  bool ap;
  if (!EspAtDrv.dhcpStateQuery(sta, ap))
    return false;
  return sta;
}

const char* WiFiClass::SSID(char* ssid) {
  uint8_t bssid[6] = {0};
  uint8_t ch = 0;
  int32_t rssi = 0;
  EspAtDrv.apQuery(ssid, bssid, ch, rssi);
  return ssid;
}

uint8_t* WiFiClass::BSSID(uint8_t* bssid) {
  uint8_t ch = 0;
  int32_t rssi = 0;
  EspAtDrv.apQuery(nullptr, bssid, ch, rssi);
  return bssid;
}

int32_t WiFiClass::RSSI() {
  uint8_t bssid[6] = {0};
  uint8_t ch = 0;
  int32_t rssi = 0;
  EspAtDrv.apQuery(nullptr, bssid, ch, rssi);
  return rssi;
}

int8_t WiFiClass::scanNetworks(WiFiApData* _apData, uint8_t _apDataSize) {
  apData = _apData;
  apDataSize = _apDataSize;
  apDataLength = EspAtDrv.listAP(apData, apDataSize);
  return apDataLength;
}

const char* WiFiClass::SSID(uint8_t index) {
  if (index >= apDataLength)
    return nullptr;
  return apData[index].ssid;

}

uint8_t WiFiClass::encryptionType(uint8_t index) {
  if (index >= apDataLength)
    return ENC_TYPE_UNKNOWN;
  return mapAtEnc2ArduinoEnc(apData[index].enc);
}

uint8_t* WiFiClass::BSSID(uint8_t index, uint8_t* bssid) {
  if (index >= apDataLength)
    return nullptr;
  memcpy(bssid, apData[index].bssid, 6);
  return bssid;
}

uint8_t WiFiClass::channel(uint8_t index) {
  if (index >= apDataLength)
    return 0;
  return apData[index].channel;
}

int32_t WiFiClass::RSSI(uint8_t index) {
  if (index >= apDataLength)
    return 0;
  return apData[index].rssi;
}

bool WiFiClass::startMDNS(const char* hostname, const char* serverName, uint16_t serverPort) {
  return EspAtDrv.mDNS(hostname, serverName, serverPort);
}

bool WiFiClass::hostByName(const char* hostname, IPAddress& result) {
  return EspAtDrv.resolve(hostname, result);
}

bool WiFiClass::ping(const char* hostname) {
  return EspAtDrv.ping(hostname);
}

bool WiFiClass::ping(IPAddress ip) {
  char s[16];
  EspAtDrv.ip2str(ip, s);
  return ping(s);
}

bool WiFiClass::sntp(int8_t timezone, const char* server1, const char* server2) {
  return EspAtDrv.sntpCfg(timezone, server1, server2);
}

unsigned long WiFiClass::getTime() {
  return EspAtDrv.sntpTime();
}

int WiFiClass::beginAP(const char *ssid, const char* passphrase, uint8_t channel, uint8_t encryptionType, uint8_t maxConnetions, bool hidden) {
  uint8_t encoding = 0; // OPEN
  switch (encryptionType) {
    case ENC_TYPE_WEP: // WEP is not supported
      return WL_AP_FAILED;
    case ENC_TYPE_TKIP:
      encoding = 2; // WPA_PSK
    break;
    case ENC_TYPE_CCMP:
      encoding = 4; // WPA_WPA2_PSK
    break;
  }
  apMaxConn = 0; // clear the ap params info cahche
  return EspAtDrv.beginSoftAP(ssid, passphrase, channel, encoding, maxConnetions, hidden, persistent) ? WL_AP_LISTENING : WL_AP_FAILED;
}

bool WiFiClass::endAP(bool pers) {
  return EspAtDrv.endSoftAP(persistent || pers);
}

bool WiFiClass::configureAP(IPAddress ip, IPAddress gateway, IPAddress subnet) {
  return EspAtDrv.softApIp(ip, gateway, subnet, persistent);
}

uint8_t* WiFiClass::apMacAddress(uint8_t* mac) {
  if (!EspAtDrv.softApMacQuery(mac)) {
    for (int i = 0; i < 5; i++) {
      mac[i] = 0;
    }
  }
  return mac;
}

const char* WiFiClass::apSSID(char* buffer) {
  uint8_t ch = 0;
  if (!EspAtDrv.softApQuery(buffer, nullptr, ch, apEnc, apMaxConn, apHidden)) {
    buffer[0] = 0;
  }
  return buffer;
}

const char* WiFiClass::apPassphrase(char* buffer) {
  uint8_t ch = 0;
  if (!EspAtDrv.softApQuery(nullptr, buffer, ch, apEnc, apMaxConn, apHidden)) {
    buffer[0] = 0;
  }
  return buffer;
}

uint8_t WiFiClass::apEncryptionType() {
  if (apMaxConn == 0) {
    apSSID(nullptr);
  }
  return mapAtEnc2ArduinoEnc(apEnc);
}

uint8_t WiFiClass::apMaxConnections() {
  if (apMaxConn == 0) {
    apSSID(nullptr);
  }
  return apMaxConn;
}

bool WiFiClass::apIsHidden() {
  if (apMaxConn == 0) {
    apSSID(nullptr);
  }
  return apHidden;
}

bool WiFiClass::apDhcpIsEnabled() {
  bool sta;
  bool ap;
  EspAtDrv.dhcpStateQuery(sta, ap);
  return ap;
}

IPAddress WiFiClass::apIP() {
  IPAddress ip;
  IPAddress gw;
  IPAddress mask;
  EspAtDrv.softApIpQuery(ip, gw, mask);
  return ip;
}

IPAddress WiFiClass::apGatewayIP() {
  IPAddress ip;
  IPAddress gw;
  IPAddress mask;
  EspAtDrv.softApIpQuery(ip, gw, mask);
  return gw;
}

IPAddress WiFiClass::apSubnetMask() {
  IPAddress ip;
  IPAddress gw;
  IPAddress mask;
  EspAtDrv.softApIpQuery(ip, gw, mask);
  return mask;
}

const char* WiFiClass::firmwareVersion(char* buffer) {
  EspAtDrv.firmwareVersion(buffer);
  return buffer;
}

EspAtDrvError WiFiClass::getLastDriverError() {
  return EspAtDrv.getLastErrorCode();
}

bool WiFiClass::deepSleep() {
  return EspAtDrv.deepSleep();
}

bool WiFiClass::reset(uint8_t resetPin) {
  return EspAtDrv.reset(resetPin);
}

uint8_t WiFiClass::mapAtEnc2ArduinoEnc(uint8_t encryptionType) {
  if (encryptionType == 0) { // WIFI_AUTH_OPEN
    encryptionType = ENC_TYPE_NONE;
  } else if (encryptionType == 1) { // WIFI_AUTH_WEP
    encryptionType = ENC_TYPE_WEP;
  } else if (encryptionType == 2) { // WIFI_AUTH_WPA_PSK
    encryptionType = ENC_TYPE_TKIP;
  } else if (encryptionType == 3 || encryptionType == 4) { // WIFI_AUTH_WPA2_PSK || WIFI_AUTH_WPA_WPA2_PSK
    encryptionType = ENC_TYPE_CCMP;
  } else {
    // unknown?
    encryptionType = ENC_TYPE_UNKNOWN;
  }
  return encryptionType;
}

WiFiClass WiFi;
