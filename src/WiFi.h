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

#ifndef _WIFI_ESP_AT_H_
#define _WIFI_ESP_AT_H_

#include <Arduino.h>
#include <IPAddress.h>
#include "WiFiEspAtConfig.h"
#include "utility/EspAtDrvTypes.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include "WiFiUdp.h"
#include "WiFiSSLClient.h"

#define WIFIESPAT_LIB_VERSION 2

enum {
  WL_NO_SHIELD = 255,
  WL_NO_MODULE = WL_NO_SHIELD,
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL,
  WL_CONNECTED,
  WL_CONNECT_FAILED,
  WL_CONNECTION_LOST,
  WL_DISCONNECTED,
  WL_AP_LISTENING,
  WL_AP_CONNECTED,
  WL_AP_FAILED
};

/* Encryption modes */
enum wl_enc_type {  /* Values map to 802.11 Cipher Algorithm Identifier */
  ENC_TYPE_WEP  = 5,
  ENC_TYPE_TKIP = 2,
  ENC_TYPE_WPA = ENC_TYPE_TKIP,
  ENC_TYPE_CCMP = 4,
  ENC_TYPE_WPA2 = ENC_TYPE_CCMP,
  ENC_TYPE_GCMP = 6,
  ENC_TYPE_WPA3 = ENC_TYPE_GCMP,
  /* ... except these two, 7 and 8 are reserved in 802.11-2007 */
  ENC_TYPE_NONE = 7,
  ENC_TYPE_AUTO = 8,

  ENC_TYPE_UNKNOWN = 255
};

class WiFiClass {
public:

  bool init(Stream& serial, int8_t resetPin = -1);
  bool init(Stream* serial, int8_t resetPin = -1); // old WiFiEsp lib compatibility

  uint8_t status();

  bool setPersistent(bool persistent = true);

  bool setAutoConnect(bool autoConnect);

#ifdef WIFIESPAT1
  int begin(const char* ssid, const char *passphrase = nullptr, const uint8_t* bssid = nullptr);
#else
  int begin(const char* ssid = nullptr, const char *passphrase = nullptr, const uint8_t* bssid = nullptr);
#endif
  int beginEnterprise(const char* ssid, uint8_t method, const char* username, const char* passphrase, const char* identity, uint8_t security);
  int disconnect(bool persistent = false);

  bool config(IPAddress local_ip, IPAddress dns_server = INADDR_NONE, IPAddress gateway = INADDR_NONE, IPAddress subnet = INADDR_NONE);

  bool setDNS(IPAddress dns_server1, IPAddress dns_server2 = INADDR_NONE);
  bool setHostname(const char* name);
  const char* hostname(char* buffer = name);

  uint8_t* macAddress(uint8_t* mac);
  IPAddress localIP();
  IPAddress gatewayIP();
  IPAddress subnetMask();
  IPAddress dnsIP(int n = 0);
  bool dhcpIsEnabled();

  // WiFi network parameters
  const char* SSID(char* _ssid = ssid); // using the default parameter will take 33 bytes of SRAM
  uint8_t* BSSID(uint8_t* bssid);
  uint8_t channel();
  int8_t RSSI();

  // enumerate WiFi access points
  int8_t scanNetworks(); // using internal array will occupy a lot of SRAM
  int8_t scanNetworks(WiFiApData* _apData, uint8_t apDataSize); // optional version
  const char* SSID(uint8_t index);
  uint8_t encryptionType(uint8_t index);
  uint8_t* BSSID(uint8_t index, uint8_t* bssid);
  uint8_t channel(uint8_t index);
  int32_t RSSI(uint8_t index);

  // additional station functions:

  bool startMDNS(const char* hostname, const char* serverName, uint16_t serverPort);

  bool hostByName(const char* hostname, IPAddress& result);

  bool ping(const char* hostname);
  bool ping(IPAddress ip);

  bool sntp(const char* server1, const char* server2 = nullptr);
  unsigned long getTime();

  // AP related functions:

  int beginAP(const char *ssid = nullptr, const char* passphrase = nullptr, uint8_t channel = 1,
      uint8_t encryptionType = ENC_TYPE_CCMP, uint8_t maxConnections = 0, bool hidden = false);
  bool endAP(bool persistent = false);

  bool configureAP(IPAddress ip, IPAddress gateway = INADDR_NONE, IPAddress subnet = INADDR_NONE);

  uint8_t* apMacAddress(uint8_t* mac);
  const char* apSSID(char*);
  const char* apPassphrase(char*);
  uint8_t apEncryptionType();
  uint8_t apMaxConnections();
  bool apIsHidden();
  bool apDhcpIsEnabled();

  IPAddress apIP();
  IPAddress apGatewayIP();
  IPAddress apSubnetMask();

  // esp8266 and esp32 compatible softAP functions
  bool softAP(const char* ssid, const char* psk = NULL, int channel = 1, int ssid_hidden = 0, int max_connection = 4);
  bool softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet);
  bool softAPdisconnect();
  IPAddress softAPIP();
  uint8_t* softAPmacAddress(uint8_t* mac);
  String softAPSSID();
  String softAPPSK();

  //
  const char* firmwareVersion(char* buffer = fwVersion);
  EspAtDrvError getLastDriverError();

  bool sleepMode(EspAtSleepMode mode);
  bool deepSleep();
  bool reset(uint8_t resetPin = -1);

private:
  uint8_t mapAtEnc2ArduinoEnc(uint8_t encryptionType);

  uint8_t state = WL_NO_MODULE;

  // this members are removed by compiler if the corresponding function is not used
  static char fwVersion[]; // static is for use as default parameter value of function
  static char ssid[];
  static char name[]; // hostname

  uint8_t apEnc;
  uint8_t apMaxConn;
  bool apHidden;

  static WiFiApData apDataInternal[];
  WiFiApData* apData;
  uint8_t apDataSize;
  uint8_t apDataLength; // count of AP
};

extern WiFiClass WiFi;

#endif
