//Sketch uses esp8266 board version 2.3.0
//Sketch uses the GxEPD library from https://github.com/ZinggJM/GxEPD
//The GxEPD library needs to be modified such that display._buffer is public instead of private. This needs to be done for the driver of each screen type you want to support.
//You may notice that this program has a few memory leaks. Those were left there on purpose, since the device reboots so often.

#include <GxEPD.h>
#include <Hash.h>
#include "debug_mode.h"
#include "admin_mode.h"
#include <pgmspace.h>
#define FIRMWARE_VERSION "2.05d"
#define DEVICE_TYPE 2
#define ADMIN_MODE_ENABLED 1
#define MAX_SLEEP 1950
#define MIN_SLEEP 10
#define ONE_DAY 86400
#define ONE_HOUR 3600
#define INITIAL_CRASH_SLEEP_SECONDS 15
#define INITIAL_DRIFT_SECONDS 30
#define ADMIN_PASSWORD "password123"
extern "C" {
#include "user_interface.h"
}

#if DEVICE_TYPE == 0
  #define X_RES 384
  #define Y_RES 640
  #define ROTATION 1
  #include <GxGDEW075T8/GxGDEW075T8.cpp>      // 7.5" b/w portrait
#elif DEVICE_TYPE == 1
  #define X_RES 400
  #define Y_RES 300
  #define ROTATION 0
  #include <GxGDEW042T2/GxGDEW042T2.cpp>      // 4.2" b/w landscape
#elif DEVICE_TYPE == 2
  #define X_RES 640
  #define Y_RES 384
  #define ROTATION 2
  #include <GxGDEW075T8/GxGDEW075T8.cpp>      // 7.5" b/w landscape
#endif

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

#include <Fonts/FreeMonoBold9pt7b.h>

GxIO_Class io(SPI, 5, 0, 2);
GxEPD_Class display(io, 2, 12);

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

ADC_MODE(ADC_VCC);  //needed to read the supply voltage

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);

// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

//number of failed attempts to connect to wifi
uint16_t attempts = 0;

// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after CRC32.
// Must use a multiple of 4 bytes of RAM.
struct {
  uint32_t crc32;
  uint32_t currentTime;
  uint32_t nextTime;
  uint32_t elapsedTime;
  int32_t driftSeconds;
  uint32_t crashSleepSeconds;
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t bssid[6]; // 6 bytes, 11 in total
  uint8_t padding;  // 1 byte,  12 in total
  char ssid[20];
  char password[20];
  char imageHash[20];
} rtcData;

struct {
  bool wifiProfile1Active;
  char baseURL[100];
  char ssid0[20];
  char ssid1[20];
  char password0[20];
  char password1[20];
  char imageKey[20];
  bool debug;
  uint8_t hash[20];
} eeprom;

String url = "";

bool readEeprom() {
  EEPROM.begin(sizeof(eeprom));
  EEPROM.get(0, eeprom);
  EEPROM.end();
  uint8_t hash[20];
  sha1((uint8_t*) &eeprom, sizeof(eeprom)-20, hash);
  bool dataCorrupted = false;
  for (int i = 0; i < 20; i++) {
    if (eeprom.hash[i] != hash[i]) {
      dataCorrupted = true;
    }
  }
  return !dataCorrupted;
}

void writeEeprom() {
  sha1((uint8_t*) &eeprom, sizeof(eeprom)-20, eeprom.hash);
  EEPROM.begin(sizeof(eeprom));
  EEPROM.put(0, eeprom);
  EEPROM.end();
  
  // Write bad crc32 so that a new image is refreshed on reboot
  rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData), sizeof(rtcData));
  // Write struct with bad crc32 to RTC memory
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
  }
}

String getMAC() {
  String mac = WiFi.macAddress();
  while(mac.indexOf(':') != -1) {
    mac.remove(mac.indexOf(':'), 1);
  }
  return mac;
}

void setURL() {
  url = eeprom.baseURL;
  url += "?mac_address=";
  url += getMAC();
  url += "&firmware=";
  url += FIRMWARE_VERSION;
  if (eeprom.debug) {
    url += "_debug";
  }
  url += "&voltage=";
  if (eeprom.debug) {
    Serial.println("Measuring Voltage...");
    Serial.print("Time in milliseconds: ");
    Serial.println(ESP.getCycleCount() / 80000);
  }
  float volts = 0.00f;
  volts = ESP.getVcc();
  url += String(volts/1024.00f);
  if (eeprom.debug) {
    Serial.print("Voltage: ");
    Serial.println(volts/1024.00f);
    Serial.print("Time in milliseconds: ");
    Serial.println(ESP.getCycleCount() / 80000);
  }
}

void crash(String reason) {
  if (rtcData.crashSleepSeconds > 3600) {
    dumpToScreen(reason);
  }

  if (eeprom.debug) {
    dumpToScreen(reason);
  }

  if (eeprom.debug) {
    Serial.println(reason);
    
    Serial.print("SSID:");
    Serial.println(WiFi.SSID());
    
    Serial.print("Free heap:");
    Serial.println(system_get_free_heap_size());
  
    Serial.print("Router MAC Address:");
    Serial.println(WiFi.BSSIDstr());
  
    Serial.print("IP address:");
    Serial.println(WiFi.localIP());
  
    Serial.print("Gateway IP:");
    Serial.println(WiFi.gatewayIP());
    
    Serial.print("RSSI:");
    Serial.println(WiFi.RSSI());
  
    Serial.print("MAC Address:");
    Serial.println(WiFi.macAddress());
  
    Serial.print("URL:");
    setURL();
    Serial.println(url);
    
    Serial.print("Last successful attempt at:");
    Serial.println(rtcData.currentTime);
  
    Serial.print("Original planned wakeup time:");
    Serial.println(rtcData.nextTime);
  
    Serial.print("Last unsuccessful attempt at:");
    Serial.println(rtcData.currentTime + rtcData.elapsedTime);
    
    Serial.print("Next attempt in:");
    Serial.print(rtcData.crashSleepSeconds * 4);
    Serial.println(" seconds");
  }
  
  rtcData.crashSleepSeconds *= 4;
  if (rtcData.crashSleepSeconds > ONE_DAY) {
    rtcData.crashSleepSeconds = ONE_DAY;
  }
  rtcData.elapsedTime += rtcData.crashSleepSeconds;
  
  // Update CRC32 of data
  rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
  // Write struct to RTC memory
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    //Serial.println("Write: ");
    //printMemory();
    //Serial.println();
  }

  ESP.deepSleep(rtcData.crashSleepSeconds * 1000000);
  
}

void sleep() {
    uint32_t sleepTime = rtcData.nextTime - rtcData.currentTime - rtcData.elapsedTime;
    if (sleepTime > MAX_SLEEP) {
      sleepTime = MAX_SLEEP;
    }
    rtcData.elapsedTime += sleepTime;

    // Update CRC32 of data
    rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
    // Write struct to RTC memory
    if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    }

    if (eeprom.debug) {
      Serial.print("currentTime: ");
      Serial.println(rtcData.currentTime);
      Serial.print("nextTime: ");
      Serial.println(rtcData.nextTime);
      Serial.print("elapsedTime: ");
      Serial.println(rtcData.elapsedTime);
      Serial.print("crashSleepSeconds: ");
      Serial.println(rtcData.crashSleepSeconds);
    }

    if (sleepTime + rtcData.driftSeconds < MIN_SLEEP) {
      if (eeprom.debug) {
        Serial.print("Sleeping for the minimum of");
        Serial.print(MIN_SLEEP);
        Serial.println(" seconds");
        Serial.print("Drift seconds: ");
        Serial.println(rtcData.driftSeconds);
      }
      ESP.deepSleep(MIN_SLEEP * 1000000);
    }
    if (eeprom.debug) {
      Serial.print("sleepTime: ");
      Serial.print(sleepTime);
      Serial.println(" seconds");
      Serial.print("Drift seconds added to the above figure: ");
      Serial.println(rtcData.driftSeconds);
      Serial.print("Time in milliseconds: ");
      Serial.println(ESP.getCycleCount() / 80000);
      Serial.println("");
    }
    ESP.deepSleep((sleepTime + rtcData.driftSeconds) * 1000000);
}

void dumpToScreen(String reason) {
  display.init();
  display.setRotation(ROTATION);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 0);
  display.println();
  
  display.println(reason);
  
  display.print("SSID:");
  display.println(WiFi.SSID());

  display.print("Router MAC Address:");
  display.println(WiFi.BSSIDstr());

  display.print("IP address:");
  display.println(WiFi.localIP());

  display.print("Gateway IP:");
  display.println(WiFi.gatewayIP());
  
  display.print("RSSI:");
  display.println(WiFi.RSSI());

  display.print("MAC Address:");
  display.println(WiFi.macAddress());

  display.print("URL:");
  setURL();
  display.println(url);
  
  display.print("Last successful attempt at:");
  display.println(rtcData.currentTime);

  display.print("Original planned wakeup time:");
  display.println(rtcData.nextTime);

  display.print("Last unsuccessful attempt at:");
  display.println(rtcData.currentTime + rtcData.elapsedTime);
  
  display.print("Next attempt in:");
  display.print(rtcData.crashSleepSeconds * 4);
  display.println(" seconds");
  
  display.update();
  for (int i = 0; i < 20; i++)
    rtcData.imageHash[i] = -1;
}

void handleRoot() {
  if (eeprom.debug) {
    Serial.println("Received request at /");
  }
  String webPage = "<!DOCTYPE HTML><html>";
  webPage += "<style>";
  webPage += "<!-- \n";
  webPage += "* { font-size:18pt; }";
  webPage += "\n -->";
  webPage += "</style>";
  webPage += "<h1>Web Configuration</h1>";
  webPage += "</p><form method='get' action='submit'>";
  webPage += "<label>SSID0: </label><input name='ssid0' value=\"";
  webPage += eeprom.ssid0;
  webPage += "\" length=20><br>";
  webPage += "<label>Password0: </label><input name='password0' value=\"";
  webPage += eeprom.password0;
  webPage += "\" length=20><br>";
  webPage += "<label>SSID1: </label><input name='ssid1' value=\"";
  webPage += eeprom.ssid1;
  webPage += "\" length=20><br>";
  webPage += "<label>Password1: </label><input name='password1' value=\"";
  webPage += eeprom.password1;
  webPage += "\" length=20><br>";
  webPage += "<label>Base URL: </label><input name='baseurl' value=\"";
  webPage += eeprom.baseURL;
  webPage += "\" length=100><br>";
  webPage += "<label>Image Key: </label><input name='imagekey' value=\"";
  webPage += eeprom.imageKey;
  webPage += "\"length=100><br>";
  webPage += "<fieldset id=\"debug\"><legend>Debug Mode</legend><ul><li>";
  webPage += "<label for=\"on\">On</label>";
  webPage += "<input name=\"debug\" value=\"1\" type=\"radio\"></li><li>";
  webPage += "<label for=\"off\">Off</label>";
  webPage += "<input name=\"debug\" value=\"0\" checked=\"true\" type=\"radio\"></li></ul></fieldset>";
  webPage += "<input type='submit'></form></html>";
  server.send(200, "text/html", webPage);
}

void handleSubmit() {
  server.send(200, "text/html", "<h1>Settings Saved</h1>");
  strcpy(eeprom.ssid0, server.arg("ssid0").c_str());
  strcpy(eeprom.password0, server.arg("password0").c_str());
  strcpy(eeprom.ssid1, server.arg("ssid1").c_str());
  strcpy(eeprom.password1, server.arg("password1").c_str());
  strcpy(eeprom.baseURL, server.arg("baseurl").c_str());
  strcpy(eeprom.imageKey, server.arg("imagekey").c_str());
  eeprom.debug = server.arg("debug").equals("1");
  writeEeprom();
  if (eeprom.debug == 1) {
    Serial.println("Received request at /submit");
    Serial.println("SSID0: ");
    Serial.println(server.arg("ssid0"));
    Serial.println("Password0: ");
    Serial.println(server.arg("password0"));
    Serial.println("SSID1: ");
    Serial.println(server.arg("ssid1"));
    Serial.println("password1: ");
    Serial.println(server.arg("password1"));
    Serial.println("baseurl: ");
    Serial.println(server.arg("baseurl"));
    Serial.println("imagekey: ");
    Serial.println(server.arg("imagekey"));
    Serial.println("Debug Mode: ");
    Serial.println(server.arg("debug"));
    Serial.println("New values have been written to Eeprom");
  }
}

void setup() {

  //if EEPROM data is bad, regenerate it
  if (!readEeprom()) {
    eeprom.wifiProfile1Active = true;
    if (eeprom.debug) {
      strcpy(eeprom.baseURL, "http://door-display.groups.et.byu.net/test/get_image.php");
    } else {
      strcpy(eeprom.baseURL, "http://door-display.groups.et.byu.net/get_image.php");
    }
    strcpy(eeprom.ssid0, "BYUSecure");
    strcpy(eeprom.ssid1, "BYU-WiFi");
    strcpy(eeprom.password0, "byuwireless");
    strcpy(eeprom.password1, "");
    strcpy(eeprom.imageKey, "hunter2");
    eeprom.debug = false;
    writeEeprom();
  }
  
  if (eeprom.debug) {
    Serial.begin(115200);
    Serial.print("Time in milliseconds: ");
    Serial.println(ESP.getCycleCount() / 80000);
    // Serial.setDebugOutput(true);

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(100);
    }
  }

  #if ADMIN_MODE_ENABLED == 1
    //todo: replace this pin with whichever pin it is that Dean actually set the switch to use
    //activate AP mode if set to do so
    pinMode(4, INPUT_PULLUP);
    if (digitalRead(4) == LOW) {
      if (eeprom.debug) {
        Serial.println();
        Serial.print("Configuring access point...\n");
      }
      display.init();
      memcpy_P(display._buffer, admin_image, sizeof(display._buffer));
      display.update();
      IPAddress apIP(192, 168, 4, 1);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      String ssid = "BYU DD ";
      ssid += getMAC();
      WiFi.softAP(ssid.c_str(), ADMIN_PASSWORD);
    
      IPAddress myIP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(myIP);
      server.on("/", handleRoot);
      server.on("/submit", handleSubmit);
      server.begin();
      Serial.println("Webserver started");
      while (true) {
        server.handleClient();
        delay(100);
      }
    }
  #endif
  
  //Don't write wifi info to flash
  WiFi.persistent(false);
  
  // Read struct from RTC memory
  if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    if (eeprom.debug) {
      //Serial.println("Read: ");
      //printMemory();
      //Serial.println();
    }
    uint32_t crcOfData = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
    if (eeprom.debug) {
      Serial.print("CRC32 of data: ");
      Serial.println(crcOfData, HEX);
      Serial.print("CRC32 read from RTC: ");
      Serial.println(rtcData.crc32, HEX);
    }
    if (crcOfData != rtcData.crc32) {
        if (eeprom.debug) {
          Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
          Serial.println("Connecting to wifi with default settings");
          Serial.print("Time in milliseconds: ");
          Serial.println(ESP.getCycleCount() / 80000);
        }
        WiFiMulti.addAP(eeprom.ssid0, eeprom.password0);
        WiFiMulti.addAP(eeprom.ssid1, eeprom.password1);
        while(WiFiMulti.run() != WL_CONNECTED) {
          Serial.print(".");
          delay(500);
        }
        rtcData.currentTime = 0;
        rtcData.nextTime = 0;
        rtcData.elapsedTime = 0;
        for (int i = 0; i < 20; i++)
          rtcData.imageHash[i] = 0;
        rtcData.driftSeconds = INITIAL_DRIFT_SECONDS;
        rtcData.crashSleepSeconds = INITIAL_CRASH_SLEEP_SECONDS;
        dumpToScreen("First boot");
    }
    else {
      //if we aren't there yet, sleep
      if (rtcData.elapsedTime + rtcData.currentTime < rtcData.nextTime) {
        sleep();
      }
      randomSeed(ESP.getCycleCount());
      if (random(30) == 1 || rtcData.crashSleepSeconds != INITIAL_CRASH_SLEEP_SECONDS) {
        WiFiMulti.addAP(eeprom.ssid0, eeprom.password0);
        WiFiMulti.addAP(eeprom.ssid1, eeprom.password1);
        if (eeprom.debug) {
          Serial.println("Connecting to wifi with default settings");
          Serial.print("Time in milliseconds: ");
          Serial.println(ESP.getCycleCount() / 80000);
        }
        while(WiFiMulti.run() != WL_CONNECTED) {
          Serial.print(".");
          delay(500);
        }
      } else {
        WiFi.begin(rtcData.ssid, rtcData.password, rtcData.channel, rtcData.bssid);
        if (eeprom.debug) {
          Serial.println("Connecting to wifi with stored settings");
          Serial.print("Time in milliseconds: ");
          Serial.println(ESP.getCycleCount() / 80000);
        }
      }
    }
  }

  /*
   * If we can find a way to get the DHCP lease time, we can safely save substantial time by using code similar to this:
  IPAddress ip(10, 10, 104, 34);
  IPAddress gateway(10, 10, 104, 1);
  IPAddress subnet( 255, 255, 248, 0);
  WiFi.config(ip, gateway, subnet);
  */
  
}

void loop() {
    // wait for WiFi connection
    if((WiFi.status() == WL_CONNECTED)) {

      if (eeprom.debug) {
        Serial.println("WiFi Connection established");
        Serial.print("IP address:");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway IP:");
        Serial.println(WiFi.gatewayIP());
        Serial.print("Subnet Mask:");
        Serial.println(WiFi.subnetMask());
        Serial.print("Time in milliseconds: ");
        Serial.println(ESP.getCycleCount() / 80000);
      }

      if (WiFi.RSSI() < -90) {
        crash("Connection Weak");
      }

      //Save wifi connection info
      strcpy(rtcData.ssid, WiFi.SSID().c_str());
      strcpy(rtcData.password, WiFi.psk().c_str());
      rtcData.channel = WiFi.channel();
      memcpy(rtcData.bssid, WiFi.BSSID(), 6);
      
      // configure server and url
      setURL();
        
      if (eeprom.debug) {
        Serial.print("[HTTP] begin...\n");
      }
      
      HTTPClient http;
      http.begin(url);

      if (eeprom.debug) {
        Serial.print("[HTTP] GET...\n");
      }
      // start connection and send HTTP header
      int httpCode = http.GET();
      if(httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          if (httpCode == 404) {
            crash("Error:404");
          }
        if (eeprom.debug) {
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        }

          // file found at server
          if(httpCode == HTTP_CODE_OK) {
              if (eeprom.debug) {
                Serial.print("Time in milliseconds: ");
                Serial.println(ESP.getCycleCount() / 80000);
              }
              int cursor = 0;
              uint8_t prevColor = 0;
              uint8_t lastEntry;
              int16_t counter = 0;
              int16_t y = 0;
              boolean initialized = false;
              // get length of document (is -1 when Server sends no Content-Length header)
              int len = http.getSize();
              if (eeprom.debug) {
                Serial.print("File Size: ");
                Serial.println(len);
              }

              //this will happen if the mac address isn't found in the database
              if (len < 50) {
                crash("File too small");
                if (eeprom.debug)
                  system_get_free_heap_size();
              }

              // create buffer for read
              uint8_t buff[128] = { 0 };

              // get tcp stream
              WiFiClient * stream = http.getStreamPtr();

              // read all data from server
              while(http.connected() && (len > 0 || len == -1)) {
                  // get available data size
                  size_t size = stream->available();

                  if(size) {
                      // read up to 128 byte
                      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                      for (int offset = 0; offset < c; offset++) {
                        if (!initialized) {           //Grab the stuff off the front of the file
                          initialized = true;
                          uint32_t predictedTime = rtcData.currentTime + rtcData.elapsedTime;
                          rtcData.currentTime = *(uint32_t*) buff;
                          if (rtcData.nextTime == *(uint32_t*) (buff + 4)) {
                            
                          } else if (rtcData.currentTime > predictedTime && rtcData.elapsedTime > 100) {
                            rtcData.driftSeconds -= 1;
                          } else if (rtcData.currentTime < predictedTime && rtcData.elapsedTime > 100) {
                            rtcData.driftSeconds += 1;
                          }
                          rtcData.nextTime = *(uint32_t*) (buff + 4);
                          rtcData.elapsedTime = 0;
                          if (eeprom.debug) {
                            Serial.print("Updated currentTime: ");
                            Serial.println(rtcData.currentTime);
                            Serial.print("Updated nextTime: ");
                            Serial.println(rtcData.nextTime);
                            Serial.print("Old imageHash: ");
                            for (int i = 0; i < 20; i++) {
                              Serial.print(String(rtcData.imageHash[i], HEX));
                            }
                            Serial.println("");
                            Serial.print("New imageHash: ");
                            for (int i = 0; i < 20; i++) {
                              Serial.print(String(*(uint8_t*) (buff + 8+i), HEX));
                            }
                            Serial.println("");
                          }
                          bool imageMatch = true;
                          for (int i = 0; i < 20; i++) {
                            if (rtcData.imageHash[i] != *(char*) (buff+8+i)) {
                              imageMatch = false;
                            }
                          }
                          if (imageMatch && rtcData.crashSleepSeconds == 15) {
                            WiFi.disconnect();
                            delay(10);
                            WiFi.forceSleepBegin();
                            delay(10);
                            sleep();
                          }
                          if (eeprom.debug) {
                            Serial.print("Initializing display; time in milliseconds: ");
                            Serial.println(ESP.getCycleCount() / 80000);
                          }
                          display.init();
                          if (eeprom.debug) {
                            Serial.print("Display initialized; time in milliseconds: ");
                            Serial.println(ESP.getCycleCount() / 80000);
                          }
                          memcpy(rtcData.imageHash, buff+8, 20);
                          rtcData.crashSleepSeconds = 15;
                          lastEntry = buff[28];
                          lastEntry = lastEntry % 2;
                          offset += 29;
                        }
                        counter = buff[offset];
                        if (counter == 255) {
                          if (lastEntry) {
                            for (int16_t i = cursor; i < cursor + 255; i++) {
                              display.drawPixel(i%X_RES, y+i/X_RES, !lastEntry);
                            }
                          }
                          cursor += 255;
                        } else if (counter == 0) {
                          lastEntry ^= 0x01;
                        } else {
                          if (lastEntry) {
                            for (int16_t i = cursor; i < cursor + counter; i++) {
                              display.drawPixel(i%X_RES, y+i/X_RES, !lastEntry);
                            }
                          }
                          lastEntry ^= 0x01;
                          cursor += counter;
                        }
                        if (cursor >= X_RES) {
                          cursor -= X_RES;
                          y++;
                        }
                      }
                      if(len > 0) {
                          len -= c;
                      }
                  }
                  delay(1);
              }
              WiFi.disconnect();
              delay(10);
              WiFi.forceSleepBegin();
              delay(10);
              if (eeprom.debug) {
                Serial.println("Calculating SHA1 hash");
                Serial.print("Time in milliseconds: ");
                Serial.println(ESP.getCycleCount() / 80000);
              }
              //hash the image, hash the password, then hash the two hashes
              //the server does the same thing, and the client compares them
              uint8_t* hash = (uint8_t*) malloc(40);
              sha1(display._buffer, X_RES*Y_RES/8, hash);
              sha1(eeprom.imageKey, strlen(eeprom.imageKey)+1, hash+20);
              uint8_t* finalHash = (uint8_t*) malloc(20);
              sha1(hash, 40, finalHash);
              Serial.println("Hash: ");
              for (int i = 0; i < 20; i++) {
                Serial.print(String(finalHash[i], HEX));
              }
              Serial.println("");
              Serial.println("");
              bool imageVerified = true;
              for (int i = 0; i < 20; i++) {
                if (rtcData.imageHash[i] != finalHash[i]) {
                 imageVerified = false;
                }
              }
              if (!imageVerified) {
                if (eeprom.debug) {
                  Serial.println("Image did not pass verification, sleeping");
                  Serial.print("Time in milliseconds: ");
                  Serial.println(ESP.getCycleCount() / 80000);
                }
                WiFi.disconnect();
                delay(10);
                WiFi.forceSleepBegin();
                delay(10);
                crash("Image failed verification test");
                sleep();
              }
              
              if (eeprom.debug) {
                Serial.println("Updating display");
                Serial.print("Time in milliseconds: ");
                Serial.println(ESP.getCycleCount() / 80000);
                memcpy_P(display._buffer, debug_image, 1120);
              }
              display.update();
              if (eeprom.debug) {
                Serial.println("Display updated, sleeping");
                Serial.print("Time in milliseconds: ");
                Serial.println(ESP.getCycleCount() / 80000);
              }
              sleep();
          if (eeprom.debug) {
              Serial.println();
              Serial.print("[HTTP] connection closed or file end.\n");
          }

          }
      } else {
      if (eeprom.debug) {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          
          crash("Error connecting to server");
      }
      }

      http.end();
    } else if (WiFi.status() == WL_CONNECT_FAILED) {
      crash("WiFi connection failed");
    } else if (WiFi.status() == WL_NO_SSID_AVAIL) {
      crash("SSID not available");
    } else if (WiFi.status() == WL_CONNECTION_LOST) {
      crash("WiFi connection lost");
    }
    attempts++;
    if (attempts == 15) {
      //WiFi.disconnect();
      delay(1);
      //WiFiMulti.addAP(WIFI_SSID0, WIFI_PASSWORD0);
      //WiFiMulti.addAP(WIFI_SSID1, WIFI_PASSWORD1);
      //WiFiMulti.run();
    } else if (attempts > 40) {
      crash("Error connecting to WiFi");
    }
    delay(500);
    if (eeprom.debug) {
      Serial.print(".");
    }
}

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
