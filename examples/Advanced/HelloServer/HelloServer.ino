/*
  HelloServer example sketch for WiFiEspAT
  using the WiFiWebServer library by Khoi Hoang
  https://github.com/khoih-prog/WiFiWebServer

  This example doesn't fit into 2 kB SRAM of ATmega328p,
  so it can't be run on Uno, classic Nano or Mini
  It works perfect on Mega, Nano Every, Nano 33 BLE, MKR, Zero, M0, ...

  created in 2015 for ESP8266WebServer library
  by Ivan Grokhotkov

  adopted in March 2020 for WiFiEspAT library
  by Juraj Andrassy https://github.com/jandrassy

*/

#define USE_WIFI_NINA false //true

#if !USE_WIFI_NINA
#include <WiFiEspAT.h>
#endif

#include <WiFiWebServer.h>

WiFiWebServer server(80);

const int led = LED_BUILTIN;

void handleRoot() 
{
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from Arduino" );
  digitalWrite(led, 0);
}

void handleNotFound() 
{
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) 
{
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  while (!Serial);

#if !USE_WIFI_NINA
  Serial1.begin(115200); 
  WiFi.init(Serial1);
#endif

  if (WiFi.status() == WL_NO_MODULE) 
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  Serial.println("");

  // Wait for connection to Wifi network set with the SetupWiFiConnection sketch
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) 
{
  server.handleClient();
}
