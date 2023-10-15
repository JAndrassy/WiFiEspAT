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

#include "EspAtDrv.h"
#include "EspAtDrvLogging.h"

#if defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
#include <itoa.h>
#define strcmp_P(a, b) strcmp((a), (b))
#define strncmp_P(a, b, n) strncmp((a), (b), (n))
#endif

//#define ESPATDRV_ASSUME_FLOW_CONTROL

const uint8_t TIMEOUT_COUNT = 3;

const uint8_t WIFI_MODE_STA = 0b01;
const uint8_t WIFI_MODE_SAP = 0b10;
const uint16_t MAX_SEND_LENGTH = 2048;

const char OK[] PROGMEM = "OK";
const char STATUS[] PROGMEM = "STATUS";
const char AT_CIPSTATUS[] PROGMEM = "AT+CIPSTATUS";
const char CIPSTATUS[] PROGMEM = "+CIPSTATUS";
const char CIPSTA[] PROGMEM = "+CIPSTA";
const char CIPAP[] PROGMEM = "+CIPAP";
const char QOUT_COMMA_QOUT[] PROGMEM = "\",\"";
const char PROCESSED[] PROGMEM = " ...processed";
const char IGNORED[] PROGMEM = " ...ignored";

#if WIFIESPAT_LOG_LEVEL >= LOG_LEVEL_DEBUG
class DebugPrint : public Print {
public:
  Print* stream;
  bool nl = true;
  virtual size_t write(uint8_t b) {
    if (nl) {
      LOG_DEBUG_PRINT_PREFIX();
      nl = false;
    }
    if (b == '\n') {
      nl = true;
    }
    LOG_OUTPUT.write(b);
    return stream->write(b);
  }
} debugPrint;
#endif

bool EspAtDrvClass::init(Stream* _serial, int8_t resetPin) {
  serial = _serial;
#if WIFIESPAT_LOG_LEVEL < LOG_LEVEL_DEBUG
  cmd = _serial;
#else
  debugPrint.stream = serial;
  cmd = &debugPrint;
#endif
  lastErrorCode = EspAtDrvError::NO_ERROR;
  return reset(resetPin);
}

bool EspAtDrvClass::reset(int8_t resetPin) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  if (resetPin >= 0) {
    LOG_INFO_PRINT(F("reset over pin "));
    LOG_INFO_PRINTLN(resetPin);
  } else {
    LOG_INFO_PRINTLN(F("soft reset"));
  }
  if (resetPin >= 0) {
    pinMode(resetPin, OUTPUT);
    delay(1);
    pinMode(resetPin, INPUT);
    readRX(PSTR("ready")); // can be missed
  } else {
    cmd->print(F("AT+RST"));
    sendCommand(PSTR("ready")); // can be missed
  }
  if (!simpleCommand(PSTR("ATE0")) || // turn off echo. must work
      !simpleCommand(PSTR("AT+CIPMUX=1")) ||  // Enable multiple connections.
      !simpleCommand(PSTR("AT+CIPRECVMODE=1"))) // Set TCP Receive Mode - passive
    return false;

#ifndef WIFIESPAT1 //AT2
   if (!simpleCommand(PSTR("AT+SYSSTORE=0"))) {// our default is persistent false
     persistent = true;
     LOG_WARN_PRINT_PREFIX();
     LOG_WARN_PRINTLN(F("Error setting store mode. Is the firmware AT2?"));
   }
#endif

  // read default wifi mode
  cmd->print(F("AT+CWMODE?"));
  if (!sendCommand(PSTR("+CWMODE")))
    return false;
  wifiMode = buffer[strlen("+CWMODE:")] - 48;
  if (!readOK())
    return false;
  wifiModeDef = wifiMode;
  return true;
}

void EspAtDrvClass::maintain() {
  lastErrorCode = EspAtDrvError::NO_ERROR;
  readRX(nullptr, false);
}

bool EspAtDrvClass::firmwareVersion(char* buff) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("fw version"));
  
  cmd->print(F("AT+GMR"));
  if (!sendCommand(PSTR("AT version:")))
    return false;
  char* start = buffer + strlen("AT version:");
  size_t l  = strchr(start, '(') - start;
  strncpy(buff, start, l);
  buff[l] = 0;
  return readOK();
}

bool EspAtDrvClass::sysPersistent(bool _persistent) {
#ifdef WIFIESPAT1
  persistent = _persistent;
  return true;
#else
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("sys store "));
  LOG_INFO_PRINTLN(_persistent ? F("on") : F("off"));

  if (sysStoreInternal(_persistent)) {
    persistent = _persistent;
    return true;
  }
  return false;
#endif
}

//AT2
bool EspAtDrvClass::sysStoreInternal(bool store) {
  cmd->print(F("AT+SYSSTORE="));
  cmd->print(store ? 1 : 0);
  return sendCommand();
}

int EspAtDrvClass::staStatus() {
  maintain();
  
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("wifi status"));

  if (wifiModeDef == 0) { // reset() was not executed successful
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("AT firmware was not initialized"));
    lastErrorCode = EspAtDrvError::NOT_INITIALIZED;
    return -1;
  }

  cmd->print((FSH_P) AT_CIPSTATUS);
  if (!sendCommand(STATUS))
    return -1;
  uint8_t status = buffer[strlen("STATUS:")] - 48;
  return readOK() ? status : -1;
}

uint8_t EspAtDrvClass::listAP(WiFiApData apData[], uint8_t size) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("list AP"));

  uint8_t mode = wifiMode | WIFI_MODE_STA; // turn on STA, leave SoftAP as it is
  if (!setWifiMode(mode, false))
    return 0;
  if (!simpleCommand(PSTR("AT+CWLAPOPT=1,31"))) // sort and first 5 values (0b0000011111)
    return false;
  cmd->print(F("AT+CWLAP"));
  uint8_t count = 0;
  bool found = sendCommand(PSTR("+CWLAP"), true, true);
  while (found) {
    WiFiApData& r = apData[count];
    const char* delims = ",:\")";
    char* tok = strtok(buffer + strlen("+CWLAP:("), delims); // <enc>
    r.enc = atoi(tok);
    tok = strtok(NULL, delims); // <ssid>
    strcpy(r.ssid, tok);
    tok = strtok(NULL, delims); // <rssi>
    r.rssi = atoi(tok);
    for (int i = 5; i >= 0; i--) {
      tok = strtok(NULL, delims); // <bssid>[i]
      r.bssid[i] = strtoul(tok, NULL, 16);
    }
    tok = strtok(NULL, delims); // <channel>
    r.channel = atoi(tok);
    count++;
    if (count == size)
      break;
    found = readRX(PSTR("+CWLAP"), true, true);
  }
  if (count == size) {
    readOK(); // skip the rest of results
  }
  return count;
}

bool EspAtDrvClass::staStaticIp(const IPAddress& ip, const IPAddress& gw, const IPAddress& nm) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("set static IP "));
  LOG_INFO_PRINT(ip);
  LOG_INFO_PRINTLN(persistent ? F(" persistent") : F(" current") );

  uint8_t mode = wifiMode | WIFI_MODE_STA; // turn on STA, leave SoftAP as it is
  if (!setWifiMode(mode, false))
    return false; // can't set ip without sta mode
#ifdef WIFIESPAT1
  if (persistent) {
#endif
    cmd->print(F("AT+CIPSTA=\""));
#ifdef WIFIESPAT1
  } else {
    cmd->print(F("AT+CIPSTA_CUR=\""));
  }
#endif  
  ip.printTo(*cmd);
  if (gw[0]) {
    cmd->print((FSH_P) QOUT_COMMA_QOUT);
    gw.printTo(*cmd);
    if (nm[0]) {
      cmd->print((FSH_P) QOUT_COMMA_QOUT);
      nm.printTo(*cmd);
    }
  }
  cmd->print('"');
  return sendCommand();
}

bool EspAtDrvClass::staDNS(const IPAddress& dns1, const IPAddress& dns2) {
  maintain();
  
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("set static DNS "));
  LOG_INFO_PRINTLN(persistent ? F("persistent") : F("current") );

  uint8_t mode = wifiMode | WIFI_MODE_STA; // turn on STA, leave SoftAP as it is
  if (!setWifiMode(mode, false))
    return false;  // can't set dns without sta mode
#ifdef WIFIESPAT1 // AT+CIPDNS doesn't work in AT 1
  if (persistent) {
    cmd->print(F("AT+CIPDNS_DEF="));
  } else {
    cmd->print(F("AT+CIPDNS_CUR="));
  }
#else
  cmd->print(F("AT+CIPDNS="));
#endif  
  if (!dns1[0]) {
    cmd->print(0);
  } else {
    cmd->print(F("1,\""));
    dns1.printTo(*cmd);
    if (dns2[0]) {
      cmd->print((FSH_P) QOUT_COMMA_QOUT);
      dns2.printTo(*cmd);
    }
    cmd->print('"');
  }
  return sendCommand();
}

bool EspAtDrvClass::staMacQuery(uint8_t* mac) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("STA MAC query "));

  cmd->print(F("AT+CIPSTAMAC?"));
  if (!sendCommand(PSTR("+CIPSTAMAC")))
    return false;
  const char* delims = ":\"";
  char* tok = strtok(buffer, delims); // +CIPSTAMAC:"
  for (int i = 5; i >= 0; i--) {
    tok = strtok(NULL, delims); // <mac>[i]
    mac[i] = strtol(tok, NULL, 16);
  }
  return readOK();
}

bool EspAtDrvClass::staIpQuery(IPAddress& ip, IPAddress& gwip, IPAddress& mask) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("STA IP query"));

  cmd->print(F("AT+CIPSTA?"));
  if (!sendCommand(CIPSTA))
    return false;
  buffer[strlen(buffer) - 1] = 0; // delete last char '\"'
  ip.fromString(buffer + strlen("+CIPSTA:ip:\""));
  if (!readRX(CIPSTA))
    return false;
  buffer[strlen(buffer) - 1] = 0;
  gwip.fromString(buffer + strlen("+CIPSTA:gateway:\""));
  if (!readRX(CIPSTA))
    return false;
  buffer[strlen(buffer) - 1] = 0;
  mask.fromString(buffer + strlen("+CIPSTA:netmask:\""));
  return readOK();
}

bool EspAtDrvClass::staDnsQuery(IPAddress& dns1, IPAddress& dns2) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("STA DNS query"));

#ifdef WIFIESPAT1 // AT+CIPDNS? has different output in AT 2
  cmd->print(F("AT+CIPDNS_CUR?"));
  if (!sendCommand(PSTR("+CIPDNS_CUR"), true, true))
    return false;
  dns1.fromString(buffer + strlen("+CIPDNS_CUR:"));
  if (!readRX(PSTR("+CIPDNS_CUR"), true, true))
    return true; // second dns is not set
  dns2.fromString(buffer + strlen("+CIPDNS_CUR:"));
#else
  cmd->print(F("AT+CIPDNS?"));
  if (!sendCommand(PSTR("+CIPDNS"), true))
    return false;
  const char* delims = ",\"";
  char* tok = strtok(buffer, delims);
  tok = strtok(NULL, delims); // skip +CIPDNS:<enabled>,
  if (tok != NULL) {
    dns1.fromString(tok);
    tok = strtok(NULL, delims);
    if (tok != NULL) {
      dns2.fromString(tok);
    }
  }
#endif
  return readOK();
}

bool EspAtDrvClass::joinAP(const char* ssid, const char* password, const uint8_t* bssid) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("join AP "));
  LOG_INFO_PRINT(ssid);
  LOG_INFO_PRINTLN(persistent ? F(" persistent") : F(" current") );

  if (!setWifiMode(wifiMode | WIFI_MODE_STA, persistent))
    return false; // can't join ap without sta mode
#ifdef WIFIESPAT1
  if (persistent) {
#endif
    cmd->print(F("AT+CWJAP"));
#ifdef WIFIESPAT1
  } else {
    cmd->print(F("AT+CWJAP_CUR"));
  }
#endif
 if (ssid) {
  cmd->print(F("=\""));
  cmd->print(ssid);
  cmd->print((FSH_P) QOUT_COMMA_QOUT);
  if (password) {
    cmd->print(password);
    if (bssid) {
      cmd->print((FSH_P) QOUT_COMMA_QOUT);
      for (int i = 5; i >= 0; i--) {
        if (bssid[i] < 16) {
          cmd->print('0');
        }
        cmd->print(bssid[i], HEX);
        if (i > 0) {
          cmd->print(':');
        }
      }
    }
  }
  cmd->print('"');
 }
  if (!sendCommand())
    return false;
  if (persistent) {
    simpleCommand(PSTR("AT+CWAUTOCONN=1"));
  }
  return true;
}

bool EspAtDrvClass::joinEAP(const char* ssid, uint8_t method, const char* identity, const char* username, const char* password, uint8_t security) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("join Enterprise AP "));
  LOG_INFO_PRINT(ssid);
  LOG_INFO_PRINTLN(persistent ? F(" persistent") : F(" current") );

  if (!setWifiMode(wifiMode | WIFI_MODE_STA, persistent))
    return false; // can't join ap without sta mode
  cmd->print(F("AT+CWJEAP=\""));
  cmd->print(ssid);
  cmd->print(',');
  cmd->print(method);
  cmd->print(',');
  if (identity) {
    cmd->print('"');
    cmd->print(identity);
    cmd->print('"');
  }
  cmd->print(',');
  if (username) {
    cmd->print('"');
    cmd->print(username);
    cmd->print('"');
  }
  cmd->print(',');
  if (password) {
    cmd->print('"');
    cmd->print(password);
    cmd->print('"');
  }
  cmd->print(',');
  cmd->print(security);
  if (!sendCommand())
    return false;
  if (persistent) {
    simpleCommand(PSTR("AT+CWAUTOCONN=1"));
  }
  return true;
}

bool EspAtDrvClass::quitAP(bool save) {
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("quit AP "));
  LOG_INFO_PRINTLN((persistent || save) ? F(" persistent") : F(" current") );

  if (wifiMode == WIFI_MODE_SAP) { // STA is off
    LOG_WARN_PRINT_PREFIX();
    LOG_WARN_PRINTLN(F("STA is off"));
    return false;
  }
#ifdef WIFIESPAT1
  if (persistent || save) {
    simpleCommand(PSTR("AT+CWAUTOCONN=0")); // don't reconnect on reset
    simpleCommand(PSTR("AT+CIPDNS_DEF=0")); // clear static DNS servers
    simpleCommand(PSTR("AT+CWDHCP=1,1")); // enable DHCP back in case static IP disabled it
  } else {
    simpleCommand(PSTR("AT+CIPDNS_CUR=0")); // clear static DNS servers
    simpleCommand(PSTR("AT+CWDHCP_CUR=1,1")); // enable DHCP back in case static IP disabled it
  }
#else
  if (persistent != save && !sysStoreInternal(save))
     return false;
  if (persistent || save) {
    simpleCommand(PSTR("AT+CWAUTOCONN=0")); // don't reconnect on reset. always stored
  }
  simpleCommand(PSTR("AT+CIPDNS=0")); // clear static DNS servers
  simpleCommand(PSTR("AT+CWDHCP=1,3")); // enable DHCP back in case static IP disabled it
  if (persistent != save && !sysStoreInternal(persistent)) {
    persistent = save;
  }
#endif
  return simpleCommand(PSTR("AT+CWQAP")); // it doesn't clear the persistent settings
}

bool EspAtDrvClass::staAutoConnect(bool autoConnect) {
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("STA auto connect "));
  LOG_INFO_PRINTLN(autoConnect ? F("on") : F("off"));

  cmd->print(F("AT+CWAUTOCONN="));
  cmd->print(autoConnect ? 1 : 0);
  return sendCommand();
}

bool EspAtDrvClass::apQuery(char* ssid, uint8_t* bssid, uint8_t& channel, int32_t& rssi) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("AP query"));

  if (ssid) {
    ssid[0] = 0;
  }

  if (wifiMode == WIFI_MODE_SAP) { // STA is off
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("STA is off"));
    return false;
  }

  cmd->print(F("AT+CWJAP?"));
  if (!sendCommand(PSTR("+CWJAP")))
    return false;

  const char* delims = ",:\"";
  char* tok = strtok(buffer + strlen("+CWJAP:\""), delims); // <ssid>
  if (ssid != nullptr) {
    strcpy(ssid, tok);
  }
  for (int i = 5; i >= 0; i--) {
    tok = strtok(NULL, delims); // <bssid>[i]
    bssid[i] = strtol(tok, NULL, 16);
  }
  tok = strtok(NULL, delims); // <channel>
  channel = atoi(tok);
  tok = strtok(NULL, delims); // <rssi>
  rssi = atol(tok);
  return readOK();
}

bool EspAtDrvClass::softApIp(const IPAddress& ip, const IPAddress& gw, const IPAddress& nm) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("set SoftAP IP "));
  LOG_INFO_PRINT(ip);
  LOG_INFO_PRINTLN(persistent ? F(" persistent") : F(" current") );

  uint8_t origMode = wifiMode;
  uint8_t mode = wifiMode | WIFI_MODE_SAP; // turn on SoftAP temporally, leave STA as it is
  if (!setWifiMode(mode))
    return false; // can't set ip without ap mode
#ifdef WIFIESPAT1
  if (persistent) {
#endif  
    cmd->print(F("AT+CIPAP=\""));
#ifdef WIFIESPAT1
  } else {
    cmd->print(F("AT+CIPAP_CUR=\""));
  }
#endif
  ip.printTo(*cmd);
  if (gw[0]) {
    cmd->print((FSH_P) QOUT_COMMA_QOUT);
    gw.printTo(*cmd);
    if (nm[0]) {
      cmd->print((FSH_P) QOUT_COMMA_QOUT);
      nm.printTo(*cmd);
    }
  }
  cmd->print('"');
  bool ok = sendCommand();
  setWifiMode(origMode);
  return ok;
}

bool EspAtDrvClass::softApMacQuery(uint8_t* mac) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("SoftAP MAC query "));

  cmd->print(F("AT+CIPAPMAC?"));
  if (!sendCommand(PSTR("+CIPAPMAC")))
    return false;
  const char* delims = ":\"";
  char* tok = strtok(buffer, delims); // +CIPAPMAC:"
  for (int i = 5; i >= 0; i--) {
    tok = strtok(NULL, delims); // <mac>[i]
    mac[i] = strtol(tok, NULL, 16);
  }
  return readOK();
}

bool EspAtDrvClass::softApIpQuery(IPAddress& ip, IPAddress& gwip, IPAddress& mask) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("SoftAP IP query"));

  cmd->print(F("AT+CIPAP?"));
  if (!sendCommand(CIPAP))
    return false;
  buffer[strlen(buffer) - 1] = 0; // delete last char '\"'
  ip.fromString(buffer + strlen("+CIPAP:ip:\""));
  if (!readRX(CIPAP))
    return false;
  buffer[strlen(buffer) - 1] = 0;
  gwip.fromString(buffer + strlen("+CIPAP:gateway:\""));
  if (!readRX(CIPAP))
    return false;
  buffer[strlen(buffer) - 1] = 0;
  mask.fromString(buffer + strlen("+CIPAP:netmask:\""));
  return readOK();
}

bool EspAtDrvClass::beginSoftAP(const char *ssid, const char* passphrase, uint8_t channel,
    uint8_t encoding, uint8_t maxConnetions, bool hidden) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("begin SoftAP "));
  LOG_INFO_PRINTLN(persistent ? F("persistent") : F("current") );

  uint8_t mode = wifiMode | WIFI_MODE_SAP; // turn on SoftAP, leave STA as it is
  if (!setWifiMode(mode, false)) // not persistent on purpose
    return false;
  if (!ssid) // only start the SoftAp as it is configured
    return true;
#ifdef WIFIESPAT1
  if (persistent) {
#endif  
    cmd->print(F("AT+CWSAP=\""));
#ifdef WIFIESPAT1
  } else {
    cmd->print(F("AT+CWSAP_CUR=\""));
  }
#endif  
  cmd->print(ssid);
  cmd->print((FSH_P) QOUT_COMMA_QOUT);
  if (passphrase) {
    cmd->print(passphrase);
  }
  cmd->print("\",");
  cmd->print(channel);
  cmd->print(',');
  cmd->print(passphrase ? encoding : 0);
  if (maxConnetions) {
    cmd->print(',');
    cmd->print(maxConnetions);
    cmd->print(',');
    cmd->print(hidden ? 1 : 0);
  }
  return sendCommand();
}

bool EspAtDrvClass::endSoftAP(bool save) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("end SoftAP "));
  LOG_INFO_PRINTLN((persistent || save) ? F("persistent") : F("current") );

  return setWifiMode(0, persistent || save); // 0 sets STA mode
}

bool EspAtDrvClass::softApQuery(char* ssid, char* passphrase, uint8_t& channel, uint8_t& encoding, uint8_t& maxConnections, bool& hidden) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("SoftAP query"));

  if (wifiMode == WIFI_MODE_STA) { // SoftAP is off
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("SoftAP is off"));
    return false;
  }

  cmd->print(F("AT+CWSAP?"));
  if (!sendCommand(PSTR("+CWSAP")))
    return false;

  const char* delims = ",";
  char* tok = strtok(buffer + strlen("+CWSAP:\""), "\""); // <ssid>
  if (ssid != nullptr) {
    strcpy(ssid, tok);
  }
  tok = strtok(NULL, delims); // <pwd>
  tok[strlen(tok) - 1] = 0; // strip "
  if (passphrase != nullptr) {
    strcpy(passphrase, tok + 1);
  }
  tok = strtok(NULL, delims); // <channel>
  channel = atoi(tok);
  tok = strtok(NULL, delims); // <cenc>
  encoding = atoi(tok);
  tok = strtok(NULL, delims); // <max con>
  maxConnections = atol(tok);
  tok = strtok(NULL, delims); // <hidden>
  hidden = atol(tok);
  return readOK();
}

bool EspAtDrvClass::serverBegin(uint16_t port, uint8_t maxConnCount, uint16_t serverTimeout, bool ssl, bool ca) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("begin server at port "));
  LOG_INFO_PRINTLN(port);

  cmd->print(F("AT+CIPSERVERMAXCONN="));
  cmd->print(maxConnCount);
  if (!sendCommand())
    return false;
  cmd->print(F("AT+CIPSERVER=1,"));
  cmd->print(port);
  if (ssl) {
    cmd->print(F(",\"SSL\","));
    cmd->print(ca);
  }
  if (!sendCommand())
    return false;
  cmd->print(F("AT+CIPSTO="));
  cmd->print(serverTimeout);
  return sendCommand();
}

bool EspAtDrvClass::serverEnd(uint16_t port) {
  maintain();
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("stop server"));
#ifdef WIFIESPAT_MULTISERVER
  cmd->print(F("AT+CIPSERVER=0,"));
  cmd->print(port);
  return sendCommand();
#else
  return simpleCommand(PSTR("AT+CIPSERVER=0"));
#endif
}

uint8_t EspAtDrvClass::clientLinkId(uint16_t serverPort, bool accept) {
  maintain();
  for (int linkId = 0; linkId < LINKS_COUNT; linkId++) {
    LinkInfo& link = linkInfo[linkId];
    if (link.isConnected() && link.isIncoming() && !link.isClosing() && !link.isAccepted()
        && (link.available || accept)) {
#ifdef WIFIESPAT_MULTISERVER
      if (!link.localPort) {
        checkLinks();
      }
      if (serverPort != link.localPort)
        continue;
#endif
      LOG_INFO_PRINT_PREFIX();
      LOG_INFO_PRINT(F("incoming linkId "));
      LOG_INFO_PRINTLN(linkId);
      if (accept) {
        link.flags |= LINK_IS_ACCEPTED;
      }
      return linkId;
    }
  }
  return NO_LINK;
}

uint8_t EspAtDrvClass::clientLinkIds(uint16_t serverPort, uint8_t linkIds[]) {
  maintain();
  uint8_t l = 0;
  for (int linkId = 0; linkId < LINKS_COUNT; linkId++) {
    LinkInfo& link = linkInfo[linkId];
    if (link.isConnected() && link.isIncoming() && !link.isClosing() && !link.isAccepted()) {
#ifdef WIFIESPAT_MULTISERVER
      if (!link.localPort) {
        checkLinks();
      }
      if (serverPort != link.localPort)
        continue;
#endif
      linkIds[l] = linkId;
      l++;
    }
  }
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(l);
  LOG_INFO_PRINTLN(F(" link ids for server"));
  return l;
}

uint8_t EspAtDrvClass::connect(const char* type, const char* host, uint16_t port,
#ifdef WIFIESPAT1
    EspAtDrvUdpDataCallback* udpDataCallback, 
#endif
    uint16_t udpLocalPort) {
  maintain();

  uint8_t linkId = freeLinkId();
  if (linkId == NO_LINK)
    return NO_LINK;

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("start "));
  LOG_INFO_PRINT(type);
  LOG_INFO_PRINT(F(" to "));
  LOG_INFO_PRINT(host);
  LOG_INFO_PRINT(':');
  LOG_INFO_PRINT(port);
  LOG_INFO_PRINT(F(" on link "));
  LOG_INFO_PRINTLN(linkId);

  LinkInfo& link = linkInfo[linkId];

  if (link.isConnected()) {
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINT(F("linkId "));
    LOG_ERROR_PRINT(linkId);
    LOG_ERROR_PRINTLN(F("is already connected."));
    lastErrorCode = EspAtDrvError::LINK_ALREADY_CONNECTED;
    return NO_LINK;
  }
  cmd->print(F("AT+CIPSTART="));
  cmd->print(linkId);
  cmd->print(F(",\""));
  cmd->print(type);
  cmd->print((FSH_P) QOUT_COMMA_QOUT);
  cmd->print(host);
  cmd->print(F("\","));
  cmd->print(port);
  if (udpLocalPort != 0) {
    cmd->print(',');
    cmd->print(udpLocalPort);
    cmd->print(",2");
#ifdef WIFIESPAT_MULTISERVER
    link.localPort = udpLocalPort;
  } else {
    link.localPort = 0;
#endif
  }
  link.flags = LINK_CONNECTED;
  if (!sendCommand()) {
    link.flags = 0;
    return NO_LINK;
  }
  if (udpLocalPort != 0) {
    link.flags |= LINK_IS_UDP_LISTNER;
#ifdef WIFIESPAT1
    link.udpDataCallback = udpDataCallback;
#endif    
  }
  return linkId;
}

bool EspAtDrvClass::close(uint8_t linkId, bool abort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("close link "));
  LOG_INFO_PRINTLN(linkId);

  LinkInfo& link = linkInfo[linkId];
  link.available = 0;
  if (!link.isConnected()) {
    LOG_INFO_PRINT_PREFIX();
    LOG_INFO_PRINTLN(F("link is already closed"));
    return true;
  }
  link.flags |= LINK_CLOSING;
  if (abort) {
    cmd->print(F("AT+CIPCLOSEMODE="));
    cmd->print(linkId);
    cmd->print(",1");
    sendCommand();
  }
  cmd->print(F("AT+CIPCLOSE="));
  cmd->print(linkId);
  return sendCommand();
}

uint16_t EspAtDrvClass::localPortQuery(uint8_t linkId) {
#ifdef WIFIESPAT_MULTISERVER
  if (linkInfo[linkId].localPort != 0)
    return linkInfo[linkId].localPort;
#endif
  IPAddress remoteIP;
  uint16_t remotePort;
  uint16_t localPort = 0;
  remoteParamsQuery(linkId, remoteIP, remotePort, localPort);
  return localPort;
}

bool EspAtDrvClass::remoteParamsQuery(uint8_t linkId, IPAddress& remoteIP, uint16_t& remotePort, uint16_t& localPort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("status of link "));
  LOG_INFO_PRINTLN(linkId);

  LinkInfo& link = linkInfo[linkId];
  if (link.isConnected()) {
    cmd->print((FSH_P) AT_CIPSTATUS);
    if (!sendCommand(STATUS))
      return false;

    while (readRX(CIPSTATUS, true, true)) {
      uint8_t id = buffer[strlen("+CIPSTATUS:")] - 48;
      if (id == linkId) {
        const char* delim = ",\"";
        char* tok = strtok(buffer, delim); // +CIPSTATUS:<link  ID>
        tok = strtok(NULL, delim); // <type>
        tok = strtok(NULL, delim); // <remote IP>
        remoteIP.fromString(tok);
        tok = strtok(NULL, delim); // <remote port>
        remotePort = atoi(tok);
        tok = strtok(NULL, delim); // <local port>
        localPort = atoi(tok);
#ifdef WIFIESPAT_MULTISERVER
        linkInfo[linkId].localPort = localPort;
#endif
        readOK();
        return true;
      }
    }
  }
  LOG_WARN_PRINT_PREFIX();
  LOG_WARN_PRINTLN(F("link is not active"));
  link.flags = 0;
  lastErrorCode = EspAtDrvError::LINK_NOT_ACTIVE;
  return false;
}

bool EspAtDrvClass::connected(uint8_t linkId) {
  maintain();
  LinkInfo& link = linkInfo[linkId];
  return link.isConnected() && !link.isClosing();
}

size_t EspAtDrvClass::availData(uint8_t linkId) {
  maintain();
  LinkInfo& link = linkInfo[linkId];
#ifndef ESPATDRV_ASSUME_FLOW_CONTROL
  if (link.available == 0 && link.isConnected() && !link.isClosing()) {
    syncLinkInfo();
  }
#endif
  return link.available;
}

size_t EspAtDrvClass::recvData(uint8_t linkId, uint8_t data[], size_t buffSize) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("get data on link "));
  LOG_INFO_PRINTLN(linkId);

  LinkInfo& link = linkInfo[linkId];
  if (link.available == 0) {
    LOG_WARN_PRINT_PREFIX();
    if (!link.isConnected()) {
      LOG_WARN_PRINTLN(F("link is not active"));
      lastErrorCode = EspAtDrvError::LINK_NOT_ACTIVE;
    } else {
      LOG_WARN_PRINTLN(F("no data for link"));
    }
    return 0;
  }

  cmd->print(F("AT+CIPRECVDATA="));
  cmd->print(linkId);
  cmd->print(',');
  cmd->print(buffSize);
  if (!sendCommand(PSTR("+CIPRECVDATA"), false)) {
#ifndef WIFIESPAT1 //AT2
    if (link.available == 0) // AT2 SSL reports more data available and closes the connection to indicate end of data
      return 0;
#endif
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINT(F("error receiving on link "));
    LOG_ERROR_PRINTLN(linkId);
    link.available = 0;
    lastErrorCode = EspAtDrvError::RECEIVE;
    return 0;
  }

#ifdef WIFIESPAT1
  size_t len = atol(buffer + strlen("+CIPRECVDATA,")); // AT 1.7.x has : after <data_len> (not matching the doc)
#else
  size_t lt = serial->readBytesUntil(',', buffer, 6);
  buffer[lt] = 0;
  size_t len = atol(buffer);
#endif
  size_t l = serial->readBytes(data, len);
  if (l != len) { //timeout
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINT(F("error receiving on link "));
    LOG_ERROR_PRINTLN(linkId);
    link.available = 0;
    lastErrorCode = EspAtDrvError::RECEIVE;
    return 0;
  }

  if (len > link.available) {
    link.available = 0;
  } else {
    link.available -= len;
  }

  readOK();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("\tgot "));
  LOG_INFO_PRINT(len);
  LOG_INFO_PRINT(F(" bytes on link "));
  LOG_INFO_PRINTLN(linkId);

  return len;
}

//for AT2
size_t EspAtDrvClass::recvDataWithInfo(uint8_t linkId, uint8_t data[], size_t buffSize, IPAddress& remoteIp, uint16_t& remotePort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("get data and info on link "));
  LOG_INFO_PRINTLN(linkId);

  LinkInfo& link = linkInfo[linkId];
  if (link.available == 0) {
    LOG_WARN_PRINT_PREFIX();
    if (!link.isConnected()) {
      LOG_WARN_PRINTLN(F("link is not active"));
      lastErrorCode = EspAtDrvError::LINK_NOT_ACTIVE;
    } else {
      LOG_WARN_PRINTLN(F("no data for link"));
    }
    return 0;
  }

  size_t len = buffSize;
  if (link.isUdpListener()) {
    if (buffSize > link.available) {
      len = link.available; // to not read data of next message
    } else if (buffSize < link.available) {
      // in AT2 UDP passive mode the message must be read at once
      LOG_ERROR_PRINT_PREFIX();
      LOG_ERROR_PRINT(F("UDP message on link "));
      LOG_ERROR_PRINT(linkId);
      LOG_ERROR_PRINT(F(" size "));
      LOG_ERROR_PRINT(link.available);
      LOG_ERROR_PRINT(F(" is larger then "));
      LOG_ERROR_PRINTLN(buffSize);
      lastErrorCode = EspAtDrvError::UDP_LARGE;
      link.available = len; // the rest of message will not be available
    }
  }
  if (!simpleCommand(PSTR("AT+CIPDINFO=1")))
    return 0;
  cmd->print(F("AT+CIPRECVDATA="));
  cmd->print(linkId);
  cmd->print(',');
  cmd->print(len);
  if (!sendCommand(PSTR("+CIPRECVDATA"), false)) {
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINT(F("error receiving on link "));
    LOG_ERROR_PRINTLN(linkId);
    link.available = 0;
    lastErrorCode = EspAtDrvError::RECEIVE;
  } else {
    size_t l = serial->readBytesUntil(',', buffer, 6);
    buffer[l] = 0;
    len = atol(buffer);
    l = serial->readBytesUntil(',', buffer, 18); // IP in quotes
    if (l > 0) {
      buffer[l - 1] = 0;
      remoteIp.fromString(buffer + 1);
    }
    l = serial->readBytesUntil(',', buffer, 6);
    buffer[l] = 0;
    remotePort = atol(buffer);

    l = serial->readBytes(data, len);
    if (l != len) { //timeout
      LOG_ERROR_PRINT_PREFIX();
      LOG_ERROR_PRINT(F("error receiving on link "));
      LOG_ERROR_PRINTLN(linkId);
      link.available = 0;
      lastErrorCode = EspAtDrvError::RECEIVE;
      len = 0;
    } else {
      if (len > link.available) {
        link.available = 0;
      } else {
        link.available -= len;
      }
      readOK();

      LOG_INFO_PRINT_PREFIX();
      LOG_INFO_PRINT(F("\tgot "));
      LOG_INFO_PRINT(len);
      LOG_INFO_PRINT(F(" bytes on link "));
      LOG_INFO_PRINTLN(linkId);
    }
  }
  simpleCommand(PSTR("AT+CIPDINFO=0"));
  return len;
}

size_t EspAtDrvClass::sendData(uint8_t linkId, const uint8_t data[], size_t len, const char* udpHost, uint16_t udpPort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("send data on link "));
  LOG_INFO_PRINTLN(linkId);

  if (!linkInfo[linkId].isConnected()) {
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("link is not connected."));
    lastErrorCode = EspAtDrvError::LINK_NOT_ACTIVE;
    return 0;
  }

  cmd->print(F("AT+CIPSEND="));
  cmd->print(linkId);
  cmd->print(',');
  cmd->print(len);
  if (udpHost != nullptr) {
    cmd->print(F(",\""));
    cmd->print(udpHost);
    cmd->print(F("\","));
    cmd->print(udpPort);
  }
  if (!sendCommand(PSTR(">")))
    return 0;

  serial->write(data, len);

  if (!readRX(PSTR("Recv ")))
    return 0;
  size_t l = atol(buffer + strlen("Recv "));
  if (!readRX(PSTR("SEND "))) // SEND OK or SEND FAIL
    return 0;
  if (strcmp_P(buffer + strlen("SEND "), OK) != 0) {// FAIL
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("failed to send data"));
    lastErrorCode = EspAtDrvError::SEND;
    return 0;
  }
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("\tsent "));
  LOG_INFO_PRINT(l);
  LOG_INFO_PRINT(F(" bytes on link "));
  LOG_INFO_PRINTLN(linkId);
  return l;
}

size_t EspAtDrvClass::sendData(uint8_t linkId, Stream& file, const char* udpHost, uint16_t udpPort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("send stream on link "));
  LOG_INFO_PRINTLN(linkId);

  if (!linkInfo[linkId].isConnected()) {
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("link is not connected."));
    lastErrorCode = EspAtDrvError::LINK_NOT_ACTIVE;
    return 0;
  }
  uint32_t len = 0;
  while (file.available()) {
    size_t l = file.available();
    if (l > MAX_SEND_LENGTH) {
      l = MAX_SEND_LENGTH;
    }
    cmd->print(F("AT+CIPSEND="));
    cmd->print(linkId);
    cmd->print(',');
    cmd->print(l);
    if (udpHost != nullptr) {
      cmd->print(F(",\""));
      cmd->print(udpHost);
      cmd->print(F("\","));
      cmd->print(udpPort);
    }
    if (!sendCommand(PSTR(">"))) {
      LOG_ERROR_PRINT_PREFIX();
      LOG_ERROR_PRINT(F("CIPSEND failed at "));
      LOG_ERROR_PRINTLN(len);
      lastErrorCode = EspAtDrvError::SEND;
      return 0;
    }
    for (size_t i = 0; i < l; i++) {
      serial->write(file.read());
    }
    if (!readRX(PSTR("Recv ")))
      return 0;
    size_t sl = atol(buffer + strlen("Recv "));
    len += sl;
    if (!readRX(PSTR("SEND ")) || strcmp_P(buffer + strlen("SEND "), OK) != 0) {// FAIL
      LOG_ERROR_PRINT_PREFIX();
      LOG_ERROR_PRINT(F("failed to send data at "));
      LOG_ERROR_PRINTLN(len);
      lastErrorCode = EspAtDrvError::SEND;
      return 0;
    }
    if (l == MAX_SEND_LENGTH && sl < MAX_SEND_LENGTH) {
      LOG_WARN_PRINT_PREFIX();
      LOG_WARN_PRINT(F("Retardment of sending data at "));
      LOG_WARN_PRINTLN(len);
    }
  }
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("\tsent "));
  LOG_INFO_PRINT(len);
  LOG_INFO_PRINT(F(" bytes on link "));
  LOG_INFO_PRINTLN(linkId);
  return len;
}

size_t EspAtDrvClass::sendData(uint8_t linkId, SendCallbackFnc callback, const char* udpHost, uint16_t udpPort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("send with callback on link "));
  LOG_INFO_PRINTLN(linkId);

  if (!linkInfo[linkId].isConnected()) {
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("link is not connected."));
    lastErrorCode = EspAtDrvError::LINK_NOT_ACTIVE;
    return 0;
  }

  cmd->print(F("AT+CIPSENDEX="));
  cmd->print(linkId);
  cmd->print(',');
  cmd->print(MAX_SEND_LENGTH);
  if (udpHost != nullptr) {
    cmd->print(F(",\""));
    cmd->print(udpHost);
    cmd->print(F("\","));
    cmd->print(udpPort);
  }
  if (!sendCommand(PSTR(">")))
    return 0;

  callback(*serial);
  delay(20); // mandatory delay before \\0
  serial->print("\\0"); //end data

  if (!readRX(PSTR("Recv ")))
    return 0;
  size_t l = atol(buffer + strlen("Recv "));
  if (!readRX(PSTR("SEND "))) // SEND OK or SEND FAIL
    return 0;
  if (strcmp_P(buffer + strlen("SEND "), OK) != 0) {// FAIL
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("failed to send data"));
    lastErrorCode = EspAtDrvError::SEND;
    return 0;
  }
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("\tsent "));
  LOG_INFO_PRINT(l);
  LOG_INFO_PRINT(F(" bytes on link "));
  LOG_INFO_PRINTLN(linkId);
  return l;
}

bool EspAtDrvClass::setHostname(const char* hostname) {
  maintain();

  uint8_t mode = wifiMode | WIFI_MODE_STA; // turn on STA, leave SoftAP as it is
  if (!setWifiMode(mode, false))
    return false;  // can't set hostname without sta mode

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINT(F("set hostname "));
  LOG_INFO_PRINTLN(hostname);

  cmd->print(F("AT+CWHOSTNAME=\""));
  cmd->print(hostname);
  cmd->print('"');
  return sendCommand();
}

bool EspAtDrvClass::hostnameQuery(char* hostname) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("hostname query"));

  cmd->print(F("AT+CWHOSTNAME?"));
  if (!sendCommand(PSTR("+CWHOSTNAME")))
    return false;
  strcpy(hostname, buffer + strlen("+CWHOSTNAME:"));
  return readOK();
}

bool EspAtDrvClass::dhcpStateQuery(bool& staDHCP, bool& softApDHCP) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("DHCP state query"));

  cmd->print(F("AT+CWDHCP?"));
  if (!sendCommand(PSTR("+CWDHCP")))
    return false;
  uint8_t state = buffer[strlen("+CWDHCP:")] - 48;
#ifdef WIFIESPAT1 // AT1 v. AT2 state bits are swapped 
  softApDHCP = state & 0b01;
  staDHCP = state & 0b10;
#else
  softApDHCP = state & 0b10;
  staDHCP = state & 0b01;
#endif  
  return readOK();
}

bool EspAtDrvClass::mDNS(const char* hostname, const char* serverName, uint16_t serverPort) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("start MDNS"));

  cmd->print(F("AT+MDNS=1,\""));
  cmd->print(hostname);
  cmd->print((FSH_P) QOUT_COMMA_QOUT);
  cmd->print(serverName);
  cmd->print("\",");
  cmd->print(serverPort);
  return sendCommand();
}

bool EspAtDrvClass::resolve(const char* hostname, IPAddress& result) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("resolve ip"));

  cmd->print(F("AT+CIPDOMAIN=\""));
  cmd->print(hostname);
  cmd->print('"');
  if (!sendCommand(PSTR("+CIPDOMAIN")))
    return false;
  result.fromString(buffer + strlen("+CIPDOMAIN:"));
  return readOK();
}

bool EspAtDrvClass::sntpCfg(const char* server1, const char* server2) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("SNTP config"));

  cmd->print(F("AT+CIPSNTPCFG=1,0,\""));
  cmd->print(server1);
  if (server2) {
    cmd->print((FSH_P) QOUT_COMMA_QOUT);
    cmd->print(server2);
  }
  cmd->print('"');
  return sendCommand();
}

unsigned long EspAtDrvClass::sntpTime() {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("SNTP time"));

#ifdef WIFIESPAT1
  cmd->print(F("AT+SNTPTIME?")); // AT LoBo firmware command
  if (!sendCommand(PSTR("+SNTPTIME")))
    return 0;
  char* tok = strtok(buffer + strlen("+SNTPTIME:"), ",");
  unsigned long res = strtoul(tok, NULL, 10);
#else
  cmd->print(F("AT+SYSTIMESTAMP?"));
  if (!sendCommand(PSTR("+SYSTIMESTAMP")))
    return 0;
  unsigned long res = strtoul(buffer + strlen("+SYSTIMESTAMP:"), NULL, 10);
#endif
  readOK();
  return res;
}

bool EspAtDrvClass::ping(const char* hostname) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("ping"));

  cmd->print(F("AT+PING=\""));
  cmd->print(hostname);
  cmd->print("\"");
  return sendCommand();
}

bool EspAtDrvClass::sleepMode(EspAtSleepMode mode) {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("set sleep mode"));

  cmd->print(F("AT+SLEEP="));
  cmd->print(mode);
  return sendCommand();
}

bool EspAtDrvClass::deepSleep() {
  maintain();

  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("deep sleep"));

  return simpleCommand(PSTR("AT+GSLP=0"));
}

void EspAtDrvClass::ip2str(const IPAddress& ip, char* s) {
  itoa(ip[0], s, 10);
  size_t l = strlen(s);
  s[l] = '.';
  itoa(ip[1], s + l + 1, 10);
  l = strlen(s);
  s[l] = '.';
  itoa(ip[2], s + l + 1, 10);
  l = strlen(s);
  s[l] = '.';
  itoa(ip[3], s + l + 1, 10);
}

uint8_t EspAtDrvClass::freeLinkId() {
  maintain();
  for (int linkId = LINKS_COUNT - 1; linkId >= 0; linkId--) {
    LinkInfo& link = linkInfo[linkId];
    if (!link.isConnected() && !link.isClosing() && !link.available) {
      LOG_INFO_PRINT_PREFIX();
      LOG_INFO_PRINT(F("free linkId is "));
      LOG_INFO_PRINTLN(linkId);
      return linkId;
    }
  }
  return NO_LINK;
}

bool EspAtDrvClass::readRX(PGM_P expected, bool bufferData, bool listItem) {

  const size_t SL_IPD = strlen("+IPD,");

  uint8_t timeout = 0;
  bool unlinkBug = false;
  uint8_t ignoredCount = 0;

  while (true) {
    size_t available = serial->available();
    if (!expected && available == 0)
      return true;
    buffer[0] = 0;
    int l = serial->readBytes(buffer, 1); // read first byte with stream's timeout
    if (!l) {// timeout or unconnected
      if (timeout == TIMEOUT_COUNT) {
        LOG_ERROR_PRINT_PREFIX();
        LOG_ERROR_PRINTLN(F("AT firmware not responding"));
        lastErrorCode = EspAtDrvError::AT_NOT_RESPONDIG;
        return false;
      }
      // next we send an invalid command to AT.
      cmd->println("?");
      // response is:
      // nothing if the firmware doesn't respond at all. readBytes will timeout again
      // "busy p..." if still processing a command. will be printed to debug output and ignored
      // ERROR if we missed some unexpected response of a current command. will be evaluated as ERROR
      timeout++;
      continue;
    }

    if (buffer[0] == '>') { // AT+CIPSEND prompt
#ifdef WIFIESPAT1
      // AT versions 1.x send a space after >. we must clear it
      serial->readBytes(buffer + 1, 1); // read byte with stream's timeout
#endif      
      buffer[1] = 0;
      l = 1;
      timeout = 0; // AT firmware responded
    } else {
      l += serial->readBytes(buffer + l, 1); // read second byte with stream's timeout
      if (l < 2)
    	  continue;  // We haven't read requested 2 bytes, something went wrong
      timeout = 0; // AT firmware responded
      if (buffer[0] == '\r' && buffer[1] == '\n') // empty line. skip it
        continue;
      char terminator = '\n';
      if (buffer[0] == '+') { // +IPD, +CIP
        if (buffer[1] == 'C' && !bufferData) { // +CIP
          terminator = ':';
#ifdef WIFIESPAT1
        } else if (buffer[1] == 'I') { // +IPD
          l += serial->readBytes(buffer + l, 4); // (+I)PD,i
          int8_t linkId = buffer[SL_IPD] - 48;
          if (linkInfo[linkId].isUdpListener()) {
            terminator = ':';
          }
#endif          
        }
      }
      l += serial->readBytesUntil(terminator, buffer + l, sizeof(buffer) - l - 1); // - 1 for terminating 0
      while (buffer[l - 1] == '\r') { // 'while' because some (ignored) messages have \r\r\n
        l--; // trim \r
      }
      buffer[l] = 0; // terminate the string
    }
    LOG_DEBUG_PRINT_PREFIX();
    LOG_DEBUG_PRINT(buffer);
    if (expected && strncmp_P(buffer, expected, strlen_P(expected)) == 0) { // startsWith
      LOG_DEBUG_PRINTLN(F(" ...matched"));
      return true;
    }
    if (strncmp_P(buffer, PSTR("+IPD,"), SL_IPD) == 0) { // startsWith
      int8_t linkId = buffer[SL_IPD] - 48;
      size_t len = atol(buffer + SL_IPD + 2);
      if (linkId >= 0 && linkId < LINKS_COUNT && len > 0) {
        LinkInfo& link = linkInfo[linkId];
#ifdef WIFIESPAT1
        if (!link.isUdpListener()) {
#endif        
          link.available = len;
          LOG_DEBUG_PRINTLN((FSH_P) PROCESSED);
#ifdef WIFIESPAT1
        } else { // UDP listener
          LOG_DEBUG_PRINT(F(":<DATA>"));
          uint8_t res = link.udpDataCallback->readRxData(serial, len);
          if (res == EspAtDrvUdpDataCallback::OK) {
            LOG_DEBUG_PRINTLN((FSH_P) PROCESSED);
          } else {
            LOG_DEBUG_PRINTLN(F(" ...error"));
            LOG_ERROR_PRINT_PREFIX();
            LOG_ERROR_PRINT(F("UDP message on link "));
            LOG_ERROR_PRINT(linkId);
            LOG_ERROR_PRINT(F(" size "));
            LOG_ERROR_PRINT(len);
            LOG_ERROR_PRINT(F(" error "));
            LOG_ERROR_PRINTLN(res);
            lastErrorCode = (EspAtDrvError)((uint8_t) EspAtDrvError::UDP_BUSY + (res - 1));
          }
        }
#endif        
#ifndef ESPATDRV_ASSUME_FLOW_CONTROL
      } else { // +IPD truncated in serial buffer overflow
        LOG_DEBUG_PRINTLN((FSH_P) IGNORED);
#endif
      }
    } else if (strcmp_P(buffer + 1, PSTR(",CONNECT")) == 0) {
      uint8_t linkId = buffer[0] - 48;
      LinkInfo& link = linkInfo[linkId];
      if (link.available == 0 && (!link.isConnected() || link.isClosing())) { // incoming connection (and we could miss CLOSED)
        link.flags = LINK_CONNECTED | LINK_IS_INCOMING;
#ifdef WIFIESPAT_MULTISERVER
        link.localPort = 0;
#endif
        LOG_DEBUG_PRINTLN((FSH_P) PROCESSED);
      } else {
        LOG_DEBUG_PRINTLN((FSH_P) IGNORED);
      }
    } else if ((strcmp_P(buffer + 1, PSTR(",CLOSED")) == 0 || strcmp_P(buffer + 1, PSTR(",CONNECT FAIL")) == 0)) {
      uint8_t linkId = buffer[0] - 48;
      linkInfo[linkId].flags = 0;
#ifndef WIFIESPAT1 //AT2
      linkInfo[linkId].available = 0; // AT2 sends CLOSED only after all data are read
#endif
      LOG_DEBUG_PRINTLN((FSH_P) PROCESSED);
      LOG_INFO_PRINT_PREFIX();
      LOG_INFO_PRINT(F("closed linkId "));
      LOG_INFO_PRINTLN(linkId);
    } else if (!strcmp_P(buffer, PSTR("ERROR")) || !strcmp_P(buffer, PSTR("FAIL"))) {
      if (unlinkBug) {
        LOG_DEBUG_PRINTLN(F(" ...UNLINK is OK"));
        return true;
      }
      if (expected == nullptr) {
        LOG_DEBUG_PRINTLN((FSH_P) IGNORED); // it is only a late response to timeout query '?'
      } else {
      LOG_DEBUG_PRINTLN(F(" ...error"));
      LOG_ERROR_PRINT_PREFIX();
      LOG_ERROR_PRINT(F("expected "));
      LOG_ERROR_PRINT((FSH_P) expected);
      LOG_ERROR_PRINT(F(" got "));
      LOG_ERROR_PRINTLN(buffer);
      lastErrorCode = EspAtDrvError::AT_ERROR;
      return false;
      }
    } else if (!strcmp_P(buffer, PSTR("No AP"))) {
      LOG_DEBUG_PRINTLN((FSH_P) PROCESSED);
      LOG_ERROR_PRINT_PREFIX();
      LOG_ERROR_PRINT(F("expected "));
      LOG_ERROR_PRINT((FSH_P) expected);
      LOG_ERROR_PRINT(F(" got "));
      LOG_ERROR_PRINTLN(buffer);
      lastErrorCode = EspAtDrvError::NO_AP;
      return false;
    } else if (!strcmp_P(buffer, PSTR("UNLINK"))) {
      unlinkBug = true;
      LOG_DEBUG_PRINTLN((FSH_P) PROCESSED);
    } else if (listItem && !strcmp_P(buffer, OK)) { // OK ends the listing of unknown items count
      LOG_DEBUG_PRINTLN(F(" ...end of list"));
      return false;
    } else {
      ignoredCount++;
      if (ignoredCount > 70) { // reset() has many ignored lines
        LOG_ERROR_PRINT_PREFIX();
        LOG_ERROR_PRINTLN(F("To much garbage on RX"));
        lastErrorCode = EspAtDrvError::AT_NOT_RESPONDIG;
        return false;
      }
      LOG_DEBUG_PRINTLN((FSH_P) IGNORED);
    }
  }
}

bool EspAtDrvClass::readOK() {
  return readRX(OK);
}

bool EspAtDrvClass::sendCommand(PGM_P expected, bool bufferData, bool listItem) {
  // AT command is already printed, but not 'entered' with "\r\n"
  LOG_DEBUG_PRINT(F(" ...sent"));
  cmd->println(); // finish AT command sending
  return expected ? readRX(expected, bufferData, listItem) : readOK();
}

bool EspAtDrvClass::simpleCommand(PGM_P command) {
  maintain();
  cmd->print((FSH_P) command);
  LOG_DEBUG_PRINT(F(" ...sent"));
  cmd->println();
  return readOK();
}

bool EspAtDrvClass::setWifiMode(uint8_t mode, bool save) {

  if (wifiModeDef == 0) { // reset() was not executed successful
    LOG_ERROR_PRINT_PREFIX();
    LOG_ERROR_PRINTLN(F("AT firmware was not initialized"));
    lastErrorCode = EspAtDrvError::NOT_INITIALIZED;
    return false;
  }

  if (mode == 0) {
    mode = WIFI_MODE_STA;
  }

  if (mode == wifiMode && (!save || mode == wifiModeDef)) // no change
    return true;

#ifdef WIFIESPAT1
  cmd->print(save ? F("AT+CWMODE=") : F("AT+CWMODE_CUR="));
  cmd->print(mode);
  if (!sendCommand())
    return false;
#else   
 if (persistent != save && !sysStoreInternal(save))
    return false;

  cmd->print(F("AT+CWMODE="));
  cmd->print(mode);
  bool ok = sendCommand();

  if (persistent != save && !sysStoreInternal(persistent)) {
    persistent = save;
  }
  if (!ok)
    return false; 
#endif    
  wifiMode = mode;
  if (save) {
    wifiModeDef = mode;
  }
  return true;
}

#ifndef ESPATDRV_ASSUME_FLOW_CONTROL
/**
 * information sent by AT firmware without request (+IPD, CONNECT, CLOSE)
 * can get lost in serial buffer overflow or on SoftwareSerial RX
 * (SoftwareSerial can't receive while transmitting).
 *
 * AT1: Since the link is closed by peer only after the data is received by AT
 * firmware, we check the receive length after link status check, to not end
 * with closed link without data size read to LinkInfo.available.
 */
bool EspAtDrvClass::syncLinkInfo() {
  if (millis() - lastSyncMillis < 500)
    return false;
  lastSyncMillis = millis();
  LOG_INFO_PRINT_PREFIX();
  LOG_INFO_PRINTLN(F("sync"));
#ifdef WIFIESPAT1
  return checkLinks() && recvLenQuery();
#else
  return recvLenQuery();
#endif  
}

bool EspAtDrvClass::recvLenQuery() {
  maintain();
  cmd->print(F("AT+CIPRECVLEN?"));
  if (!sendCommand(PSTR("+CIPRECVLEN")))
    return false;
  const char* delim = ",";
  char* tok = strtok(buffer + strlen("+CIPRECVLEN:"), delim);
  for (int linkId = 0; linkId < LINKS_COUNT; linkId++) {
    if (tok == NULL)
      break;
    if (strlen(tok) > 0) {
#ifdef WIFIESPAT1
      linkInfo[linkId].available = atol(tok);
#else
      LinkInfo& link = linkInfo[linkId];
      if (tok[0] == '-') { // AT V2 sends -1 for inactive links
        link.flags = 0;
        link.available = 0;
      } else {
        if (!link.isConnected() || link.isClosing()) { // missed incoming connection
          link.flags = LINK_CONNECTED | LINK_IS_INCOMING;
        }
        link.available = atol(tok);
      }
#endif
    }
    tok = strtok(NULL, delim);
  }
  return readOK();
}
#endif

#if !defined(ESPATDRV_ASSUME_FLOW_CONTROL) || defined(WIFIESPAT_MULTISERVER)
bool EspAtDrvClass::checkLinks() {
  maintain();
  cmd->print((FSH_P) AT_CIPSTATUS);
  if (!sendCommand(STATUS))
    return false;
  bool ok[LINKS_COUNT] = {false};
  while (readRX(CIPSTATUS, true, true)) {
    uint8_t linkId = buffer[strlen("+CIPSTATUS:")] - 48;
    ok[linkId] = true;
#ifdef WIFIESPAT_MULTISERVER
    const char* delim = ",\"";
    char* tok = strtok(buffer, delim); // +CIPSTATUS:<link  ID>
    tok = strtok(NULL, delim); // <type>
    tok = strtok(NULL, delim); // <remote IP>
    tok = strtok(NULL, delim); // <remote port>
    tok = strtok(NULL, delim); // <local port>
    linkInfo[linkId].localPort = atoi(tok);
#endif
  }
  for (int linkId = 0; linkId < LINKS_COUNT; linkId++) {
    LinkInfo& link = linkInfo[linkId];
    if (ok[linkId]) {
      if (!link.isConnected() || link.isClosing()) { // missed incoming connection
        link.flags = LINK_CONNECTED | LINK_IS_INCOMING;
      }
    } else { // not connected
      link.flags = 0;
    }
  }
  return true;
}
#endif

EspAtDrvClass EspAtDrv;
