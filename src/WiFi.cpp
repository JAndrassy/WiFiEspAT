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

// these statics are removed by compiler, if not used
char WiFiClass::fwVersion[15] = {0};
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

bool WiFiClass::setPersistent(bool persistent) {
  return EspAtDrv.sysPersistent(persistent);
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
  bool ok = EspAtDrv.joinAP(ssid, passphrase, bssid);
  state = ok ? WL_CONNECTED : WL_CONNECT_FAILED;
  return state;
}

int WiFiClass::beginEnterprise(const char* ssid, uint8_t method, const char* username, const char* passphrase, const char* identity, uint8_t security) {
  bool ok = EspAtDrv.joinEAP(ssid, method,  identity, passphrase, username, security);
  state = ok ? WL_CONNECTED : WL_CONNECT_FAILED;
  return state;
}

int WiFiClass::disconnect(bool persistent) {
  if (EspAtDrv.quitAP(persistent)) {
    state = WL_DISCONNECTED;
  }
  return state;
}

bool WiFiClass::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet) {

  if (local_ip == INADDR_NONE)
    return EspAtDrv.staEnableDHCP();

  if (dns_server == INADDR_NONE) {
    dns_server = local_ip;
    dns_server[3] = 1;
  }
  return EspAtDrv.staStaticIp(local_ip, gateway, subnet) && setDNS(dns_server);
}

bool WiFiClass::setDNS(IPAddress dns_server1, IPAddress dns_server2) {
  return EspAtDrv.setDNS(dns_server1, dns_server2);
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

IPAddress WiFiClass::dnsIP(int n) {
  IPAddress dns1;
  IPAddress dns2;
  EspAtDrv.dnsQuery(dns1, dns2);
  switch (n) {
    case 0:
      return dns1;
    case 1:
      return dns2;
  }
  return IPAddress(0, 0, 0, 0);
}

bool WiFiClass::dhcpIsEnabled() {
  bool sta;
  bool ap;
  bool eth;
  if (!EspAtDrv.dhcpStateQuery(sta, ap, eth))
    return false;
  return sta;
}

const char* WiFiClass::SSID(char* ssid) {
  uint8_t bssid[6] = {0};
  uint8_t ch = 0;
  int8_t rssi = 0;
  EspAtDrv.apQuery(ssid, bssid, ch, rssi);
  return ssid;
}

uint8_t* WiFiClass::BSSID(uint8_t* bssid) {
  uint8_t ch = 0;
  int8_t rssi = 0;
  EspAtDrv.apQuery(nullptr, bssid, ch, rssi);
  return bssid;
}

uint8_t WiFiClass::channel() {
  uint8_t bssid[6] = {0};
  uint8_t ch = 0;
  int8_t rssi = 0;
  EspAtDrv.apQuery(nullptr, bssid, ch, rssi);
  return ch;
}

int8_t WiFiClass::RSSI() {
  uint8_t bssid[6] = {0};
  uint8_t ch = 0;
  int8_t rssi = 0;
  EspAtDrv.apQuery(nullptr, bssid, ch, rssi);
  return rssi;
}

int8_t WiFiClass::scanNetworks() {
  return scanNetworks(apDataInternal, WIFIESPAT_INTERNAL_AP_LIST_SIZE);
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

bool WiFiClass::sntp(const char* server1, const char* server2) {
  return EspAtDrv.sntpCfg(server1, server2);
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
  return EspAtDrv.beginSoftAP(ssid, passphrase, channel, encoding, maxConnetions, hidden) ? WL_AP_LISTENING : WL_AP_FAILED;
}

bool WiFiClass::endAP(bool pers) {
  return EspAtDrv.endSoftAP(pers);
}

bool WiFiClass::configureAP(IPAddress ip, IPAddress gateway, IPAddress subnet) {
  return EspAtDrv.softApIp(ip, gateway, subnet);
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
  bool eth;
  EspAtDrv.dhcpStateQuery(sta, ap, eth);
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

bool WiFiClass::softAP(const char* ssid, const char* psk, int channel, int ssid_hidden, int max_connection) {
  return beginAP(ssid, psk, channel, 0, max_connection, ssid_hidden);
}

bool WiFiClass::softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet) {
  return configureAP(local_ip, gateway, subnet);
}

bool WiFiClass::softAPdisconnect() {
  return endAP(false);
}

IPAddress WiFiClass::softAPIP() {
  return apIP();
}

uint8_t* WiFiClass::softAPmacAddress(uint8_t* mac) {
  return apMacAddress(mac);
}

String WiFiClass::softAPSSID() {
  char ssid[33];
  return apSSID(ssid);
}

String WiFiClass::softAPPSK() {
  char pass[64];
  return apPassphrase(pass);
}

const char* WiFiClass::firmwareVersion(char* buffer) {
  EspAtDrv.firmwareVersion(buffer);
  return buffer;
}

EspAtDrvError WiFiClass::getLastDriverError() {
  return EspAtDrv.getLastErrorCode();
}

bool WiFiClass::sleepMode(EspAtSleepMode mode) {
  return EspAtDrv.sleepMode(mode);
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
