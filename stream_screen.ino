#include <GxEPD.h>

#define DEVICE_TYPE 3
#define DEBUG 1
#define MAX_SLEEP 2000
#define MIN_SLEEP 10

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

ESP8266WiFiMulti WiFiMulti;

// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

// helper function to dump memory contents as hex
void printMemory();

//number of failed attempts to connect to wifi
uint16_t attempts = 0;

// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after CRC32.
// We use byte array as an example.
struct {
  uint32_t crc32;
  byte data[508];
} rtcData;

uint8_t* currentTime;
uint8_t* nextTime;
uint8_t* elapsedTime;
uint8_t* driftSeconds;

void sleep() {
    uint32_t sleepTime = *((uint32_t*) nextTime) - *((uint32_t*) currentTime) - *((uint32_t*) elapsedTime);
    if (sleepTime > MAX_SLEEP) {
      sleepTime = MAX_SLEEP;
    }
    *((uint32_t*) elapsedTime) += sleepTime;

    //write times to rtcData struct
    rtcData.data[0] = currentTime[0];
    rtcData.data[1] = currentTime[1];
    rtcData.data[2] = currentTime[2];
    rtcData.data[3] = currentTime[3];
    rtcData.data[4] = nextTime[0];
    rtcData.data[5] = nextTime[1];
    rtcData.data[6] = nextTime[2];
    rtcData.data[7] = nextTime[3];
    rtcData.data[8] = elapsedTime[0];
    rtcData.data[9] = elapsedTime[1];
    rtcData.data[10] = elapsedTime[2];
    rtcData.data[11] = elapsedTime[3];
    rtcData.data[12] = driftSeconds[0];
    rtcData.data[13] = driftSeconds[1];
    rtcData.data[14] = driftSeconds[2];
    rtcData.data[15] = driftSeconds[3];
    
    // Generate new data set for the struct
    for (int i = 16; i < sizeof(rtcData); i++) {
      rtcData.data[i] = random(0, 128);
    }
    // Update CRC32 of data
    rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
    // Write struct to RTC memory
    if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
      //Serial.println("Write: ");
      //printMemory();
      //Serial.println();
    }

    free(currentTime);
    free(nextTime);
    free(elapsedTime);
    if (sleepTime + *((int32_t*) driftSeconds) < MIN_SLEEP) {
      #if DEBUG == 1
        Serial.print("Sleeping for the minimum of");
        Serial.print(MIN_SLEEP);
        Serial.println(" seconds");
        Serial.print("Drift seconds: ");
        Serial.println(*((int32_t*) driftSeconds));
      #endif
      ESP.deepSleep(MIN_SLEEP * 1000000);
    }
    #if DEBUG == 1
      Serial.print("Sleeping for ");
      Serial.print((sleepTime + *((int32_t*) driftSeconds)));
      Serial.println(" seconds");
      Serial.print("Drift seconds: ");
      Serial.println(*((int32_t*) driftSeconds));
    #endif
    ESP.deepSleep((sleepTime + *((int32_t*) driftSeconds)) * 1000000);
    free(driftSeconds);
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
  display.println("SSID:BYUSecure");
  byte mac[6];
  WiFi.macAddress(mac);
  display.print("MAC:");
  display.print(mac[5],HEX);
  display.print(":");
  display.print(mac[4],HEX);
  display.print(":");
  display.print(mac[3],HEX);
  display.print(":");
  display.print(mac[2],HEX);
  display.print(":");
  display.print(mac[1],HEX);
  display.print(":");
  display.println(mac[0],HEX);
  
  display.update();
}

void setup() {

    #if DEBUG == 1
      Serial.begin(921600);
      // Serial.setDebugOutput(true);
  
      Serial.println();
      Serial.println();
      Serial.println();
    #endif

    currentTime = (uint8_t*) malloc(4);
    nextTime = (uint8_t*) malloc(4);
    elapsedTime = (uint8_t*) malloc(4);
    driftSeconds = (uint8_t*) malloc(4);

    // Read struct from RTC memory
    if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
      #if DEBUG == 1
        Serial.println("Read: ");
        printMemory();
        Serial.println();
      #endif
      currentTime[0] = rtcData.data[0];
      currentTime[1] = rtcData.data[1];
      currentTime[2] = rtcData.data[2];
      currentTime[3] = rtcData.data[3];
      nextTime[0] = rtcData.data[4];
      nextTime[1] = rtcData.data[5];
      nextTime[2] = rtcData.data[6];
      nextTime[3] = rtcData.data[7];
      elapsedTime[0] = rtcData.data[8];
      elapsedTime[1] = rtcData.data[9];
      elapsedTime[2] = rtcData.data[10];
      elapsedTime[3] = rtcData.data[11];
      driftSeconds[0] = rtcData.data[12];
      driftSeconds[1] = rtcData.data[13];
      driftSeconds[2] = rtcData.data[14];
      driftSeconds[3] = rtcData.data[15];
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
          *((uint32_t*) currentTime) = 0;
          *((uint32_t*) nextTime) = 0;
          *((uint32_t*) elapsedTime) = 0;
          *((int32_t*) driftSeconds) = 0;
          dumpToScreen("First boot");
      }
      else {
        #if DEBUG == 1
          Serial.println("CRC32 check ok, data is probably valid.");
        #endif
      }
    }
    
    //if we aren't there yet, sleep
    if (*((uint32_t*) elapsedTime) + *((uint32_t*) currentTime) < *((uint32_t*) nextTime)) {
      sleep();
    }

    for(uint8_t t = 4; t > 0; t--) {
      #if DEBUG == 1
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
      #endif
    }

    WiFiMulti.addAP("BYUSecure", "byuwireless");

}

void loop() {
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {

        HTTPClient http;

    #if DEBUG == 1
        Serial.print("[HTTP] begin...\n");
    #endif

        // configure server and url
        http.begin("http://door-display.groups.et.byu.net/get_image.php?mac_address=30aea40b54b&voltage=0");

    #if DEBUG == 1
        Serial.print("[HTTP] GET...\n");
    #endif
        // start connection and send HTTP header
        int httpCode = http.GET();
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
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
                          if (cursor == 0 && offset == 0) {
                            uint32_t predictedTime = *((uint32_t*) currentTime) + *((uint32_t*) elapsedTime);
                            currentTime[0] = buff[0];
                            currentTime[1] = buff[1];
                            currentTime[2] = buff[2];
                            currentTime[3] = buff[3];
                            nextTime[0] = buff[4];
                            nextTime[1] = buff[5];
                            nextTime[2] = buff[6];
                            nextTime[3] = buff[7];
                            if (*((uint32_t*) currentTime) > predictedTime) {
                              *((int32_t*) driftSeconds) -= 1;
                            } else {
                              *((int32_t*) driftSeconds) += 1;
                            }
                            *((uint32_t*) elapsedTime) = 0;
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
            sleep();
        #endif
        }

        http.end();
    }
    attempts++;
    delay(1000);
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

void printMemory() {
  char buf[3];
  for (int i = 0; i < sizeof(rtcData); i++) {
    sprintf(buf, "%02X", rtcData.data[i]);
    Serial.print(buf);
    if ((i + 1) % 32 == 0) {
      Serial.println();
    }
    else {
      Serial.print(" ");
    }
  }
  Serial.println();
}
