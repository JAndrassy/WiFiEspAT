/*
  SD Web Server example sketch for WiFiEspAT

  the example demonstrates the write(file) and write() with callback functions

  this example doesn't fit into 2 kB SRAM, so it can't be run on Uno, Nano, Mini

  created in December 2019 for WiFiEspAT library
  by Juraj Andrassy https://github.com/jandrassy

*/

#include <WiFiEspAT.h>
#include <SD.h>
#include <StreamLib.h> // install in Library Manager. Used to generate HTML of directory listing

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

const int SDCARD_CS = 4;

WiFiServer server(80);

void setup() {

  Serial.begin(115200);
  while (!Serial);

  if (!SD.begin(SDCARD_CS)) {
    Serial.println(F("SD card initialization failed!"));
    // don't continue
    while (true);
  }

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);


  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(F("Communication with WiFi module failed!"));
    // don't continue
    while (true);
  }

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  Serial.println(F("Waiting for connection to WiFi"));
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  server.begin(3);

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.println(F("Connected to WiFi network."));
  Serial.print(F("To access the server, enter \"http://"));
  Serial.print(ip);
  Serial.println(F("/\" in web browser."));
}

void loop() {

  static File file; // must by static to be accessible by the lambda functions
  static char fn[32];

  WiFiClient client = server.available();

  if (client && client.connected()) {
    if (client.find(' ')) { // GET /fn HTTP/1.1
      int l = client.readBytesUntil(' ', fn, sizeof(fn) - 1); // read the filename from URL
      fn[l] = 0;
      while (client.read() != -1); // discard the rest of the request
      file = SD.open(fn);
      if (!file) { // file was not found
        client.write([](Print& p) { // anonymous lambda function to be called by the library
          p.println(F("HTTP/1.1 404 Not Found"));
          p.println(F("Connection: close"));
          p.print(F("Content-Length: "));
          p.println(strlen(" not found") + strlen(fn));
          p.println();
          p.print(fn);
          p.print(F(" not found"));
        });
      } else if (file.isDirectory()) {
        client.write([](Print& p) { // anonymous lambda function to be called by the library
          p.println(F("HTTP/1.1 200 OK"));
          p.println(F("Connection: close"));
          p.println(F("Content-Type: text/html"));
          p.println(F("Transfer-Encoding: chunked"));
          p.println();
          char buff[64]; // buffer for chunks
          ChunkedPrint chunked(p, buff, sizeof(buff));
          chunked.begin();
          chunked.printf(F("<!DOCTYPE html>\r\n<html>\r\n<body>\r\n<h3>Folder '%s'</h3>\r\n"), fn);
          while (true) {
            File entry = file.openNextFile();
            if (!entry)
              break;
            if (strcmp(fn, "/") == 0) {
              chunked.printf(F("<a href='%s'>"), entry.name());
            } else {
              chunked.printf(F("<a href='%s/%s'>"), fn, entry.name());
            }
            chunked.print(entry.name());
            if (entry.isDirectory()) {
              chunked.println(F("/</a><br>"));
            } else  {
              chunked.printf(F("</a> (%ld b)<br>\r\n"), entry.size());
            }
            entry.close();
          }
          chunked.println(F("</body>\r\n</html>"));
          chunked.end();
        });
      } else {
        client.write([](Print& p) { // anonymous lambda function to be called by the library
          p.println(F("HTTP/1.1 200 OK"));
          p.println(F("Connection: close"));
          p.print(F("Content-Length: "));
          p.println(file.size());
          p.print(F("Content-Type: "));
          const char* ext = strchr(file.name(), '.');
          p.println(getContentType(ext));
          p.println();
        });
        client.write(file); // send the file as body of the response
        file.close();
      }
    }
    client.stop();
  }
}

const char* getContentType(const char* ext){
  if (!strcmp(ext, ".HTM"))
    return "text/html";
  if (!strcmp(ext, ".CSS"))
    return "text/css";
  if (!strcmp(ext, ".JS"))
    return "application/javascript";
  if (!strcmp(ext, ".PNG"))
    return "image/png";
  if (!strcmp(ext, ".GIF"))
    return "image/gif";
  if (!strcmp(ext, ".JPG"))
    return "image/jpeg";
  if (!strcmp(ext, ".XML"))
    return "text/xml";
  return "text/plain";
}
