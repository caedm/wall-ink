//Sketch uses esp8266 board version 2.3.0

#include <GxEPD.h>

#define DEVICE_TYPE 2
#define DEBUG 1
#define MAX_SLEEP 1950
#define MIN_SLEEP 10
#define ONE_DAY 86400

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
  #define ROTATION 0
  #include <GxGDEW075T8/GxGDEW075T8.cpp>      // 7.5" b/w landscape
#elif DEVICE_TYPE == 3
  #define X_RES 640
  #define Y_RES 384
  #define ROTATION 0
  #include <GxGDEW075T8/GxGDEW075T8.cpp>      // 7.5" b/w landscape
#endif

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

#include <Fonts/FreeMonoBold9pt7b.h>

GxIO_Class io(SPI, 15, 0, 2);
GxEPD_Class display(io, 2, 12);

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

ADC_MODE(ADC_VCC);  //needed to read the supply voltage

ESP8266WiFiMulti WiFiMulti;

// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

//number of failed attempts to connect to wifi
uint16_t attempts = 0;

// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after CRC32.
// We use byte array as an example.
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
} rtcData;

String url = "";

void setURL() {
  url = "";
  url += "http://door-display.groups.et.byu.net/get_image.php?mac_address=";
  //url += "http://10.2.124.205/~johnathan/get_image.php?mac_address=";
  String mac = WiFi.macAddress();
  while(mac.indexOf(':') != -1) {
    mac.remove(mac.indexOf(':'), 1);
  }
  url += mac;
  url += "&voltage=";
  float volts = 0.00f;
  volts = ESP.getVcc();
  url += String(volts/1024.00f);
  #if DEBUG == 1
    Serial.print("Voltage: ");
    Serial.println(volts/1024.00f);
  #endif
}
/*
void setURL() {
  url = "http://10.2.124.205/~johnathan/60019431677B.compressed";
}
*/

void crash(String reason) {
  if (rtcData.crashSleepSeconds > 3600) {
    dumpToScreen(reason);
  }

  #if DEBUG == 1
    Serial.println();
    Serial.println("Unable to retrieve image from server");
    
    Serial.println(reason);
    
    Serial.print("SSID:");
    Serial.println(WiFi.SSID());
  
    Serial.print("Router MAC Address:");
    Serial.println(WiFi.BSSIDstr());
  
    Serial.print("IP address:");
    Serial.println(WiFi.localIP());
  
    Serial.print("Gateway IP:");
    Serial.println(WiFi.gatewayIP());
    
    Serial.print("RSSI:");
    Serial.print(WiFi.RSSI());
    Serial.println("dbm");
  
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
  #endif
  
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
      //Serial.println("Write: ");
      //printMemory();
      //Serial.println();
    }

    #if DEBUG == 1
      Serial.print("currentTime: ");
      Serial.println(rtcData.currentTime);
      Serial.print("nextTime: ");
      Serial.println(rtcData.nextTime);
      Serial.print("elapsedTime: ");
      Serial.println(rtcData.elapsedTime);
      Serial.print("crashSleepSeconds: ");
      Serial.println(rtcData.crashSleepSeconds);
    #endif

    //free(currentTime);
    //free(nextTime);
    //free(elapsedTime);
    //free(crashSleepSeconds);
    if (sleepTime + rtcData.driftSeconds < MIN_SLEEP) {
      #if DEBUG == 1
        Serial.print("Sleeping for the minimum of");
        Serial.print(MIN_SLEEP);
        Serial.println(" seconds");
        Serial.print("Drift seconds: ");
        Serial.println(rtcData.driftSeconds);
      #endif
      ESP.deepSleep(MIN_SLEEP * 1000000);
    }
    #if DEBUG == 1
      Serial.print("sleepTime: ");
      Serial.print(sleepTime);
      Serial.println(" seconds");
      Serial.print("Drift seconds added to the above figure: ");
      Serial.println(rtcData.driftSeconds);
    #endif
    
    ESP.deepSleep((sleepTime + rtcData.driftSeconds) * 1000000);
    ///free(driftSeconds);
}

void dumpToScreen(String reason) {
  display.init();
  display.setRotation(ROTATION);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 0);
  
  display.setFont(&FreeMonoBold9pt7b);
  display.println();
  display.println("Unable to retrieve image from server");
  display.setFont(&FreeMonoBold9pt7b);
  
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
  display.print(WiFi.RSSI());
  display.println("dbm");

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
}

void setup() {
    #if DEBUG == 1
      Serial.begin(921600);
      // Serial.setDebugOutput(true);

      for(uint8_t t = 4; t > 0; t--) {
          Serial.printf("[SETUP] WAIT %d...\n", t);
          Serial.flush();
          delay(100);
      }
    #endif

    //Don't write wifi info to flash
    WiFi.persistent(false);
    
    // Read struct from RTC memory
    if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
      #if DEBUG == 1
        //Serial.println("Read: ");
        //printMemory();
        //Serial.println();
      #endif
      uint32_t crcOfData = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
      #if DEBUG == 1
        Serial.print("CRC32 of data: ");
        Serial.println(crcOfData, HEX);
        Serial.print("CRC32 read from RTC: ");
        Serial.println(rtcData.crc32, HEX);
      #endif
      if (crcOfData != rtcData.crc32) {
        #if DEBUG == 1
          Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
        #endif
          WiFi.begin("BYUSecure", "byuwireless");
          rtcData.currentTime = 0;
          rtcData.nextTime = 0;
          rtcData.elapsedTime = 0;
          rtcData.driftSeconds = 30;
          rtcData.crashSleepSeconds = 15;
          dumpToScreen("First boot");
      }
      else {
        #if DEBUG == 1
          Serial.println("CRC32 check ok, data is probably valid.");
        #endif
        WiFi.begin("BYUSecure", "byuwireless", rtcData.channel, rtcData.bssid);
      }
    }
    
    //if we aren't there yet, sleep
    if (rtcData.elapsedTime + rtcData.currentTime < rtcData.nextTime) {
      sleep();
    }

}

void loop() {
    // wait for WiFi connection
    if((WiFi.status() == WL_CONNECTED)) {

      //Save wifi connection info
      rtcData.channel = WiFi.channel();
      memcpy(rtcData.bssid, WiFi.BSSID(), 6);
      
      HTTPClient http;

      #if DEBUG == 1
        Serial.print("[HTTP] begin...\n");
      #endif

        // configure server and url
        setURL();
        http.begin(url);

      #if DEBUG == 1
        Serial.print("[HTTP] GET...\n");
      #endif
        // start connection and send HTTP header
        int httpCode = http.GET();
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            if (httpCode == 404) {
              crash("Error:404");
            }
          #if DEBUG == 1
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
          #endif

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                display.init();
                display.setRotation(ROTATION);
                int cursor = 0;
                uint8_t prevColor = 0;
                uint8_t lastEntry;
                int16_t counter = 0;
                int16_t y = 0;
                boolean initialized = false;
                // get length of document (is -1 when Server sends no Content-Length header)
                int len = http.getSize();

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
                            rtcData.nextTime = *(uint32_t*) (buff + 4);
                            #if DEBUG == 1
                              Serial.print("Updated currentTime: ");
                              Serial.println(rtcData.currentTime);
                              Serial.print("Updated nextTime: ");
                              Serial.println(rtcData.nextTime);
                            #endif
                            rtcData.crashSleepSeconds = 15;
                            if (rtcData.currentTime > predictedTime && rtcData.elapsedTime > 100) {
                              rtcData.driftSeconds -= 1;
                            } else if (rtcData.currentTime < predictedTime && rtcData.elapsedTime > 100) {
                              rtcData.driftSeconds += 1;
                            }
                            rtcData.elapsedTime = 0;
                            lastEntry = buff[8];
                            lastEntry = lastEntry % 2;
                            offset += 9;
                          }
                          counter = buff[offset];
                          if (counter == 255) {
                            for (int16_t i = cursor; i < cursor + 255; i++) {
                              #if DEBUG == 1
                                  //Serial.print(lastEntry);
                                  //if (i % X_RES == 0)
                                    //Serial.println("");
                              #endif
                              display.drawPixel(i%X_RES, y+i/X_RES, lastEntry);
                            }
                            cursor += 255;
                          } else {
                            for (int16_t i = cursor; i < cursor + counter; i++) {
                              #if DEBUG == 1
                                  //Serial.print(lastEntry);
                                  //if (i % X_RES == 0)
                                    //Serial.println("");
                              #endif
                              display.drawPixel(i%X_RES, y+i/X_RES, lastEntry);
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
                display.update();
                sleep();
            #if DEBUG == 1
                Serial.println();
                Serial.print("[HTTP] connection closed or file end.\n");
            #endif

            }
        } else {
        #if DEBUG == 1
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            
            crash("Error connecting to server");
        #endif
        }

        http.end();
    }
    attempts++;
    if (attempts > 75) {
      WiFi.disconnect();
      delay(10);
      WiFi.forceSleepBegin();
      delay(10);
      WiFi.forceSleepWake();
      delay(10);
      WiFi.begin("BYUSecure", "byuwireless");
    } else if (attempts > 200) {
      crash("Error connecting to WiFi");
    }
    delay(100);
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
