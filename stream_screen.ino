#include <GxEPD.h>

#define DEVICE_TYPE 3
#define DEBUG 0

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

GxIO_Class io(SPI, 15, 0, 2);
GxEPD_Class display(io, 2, 12);

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;

void setup() {

#if DEBUG == 1
    Serial.begin(921600);
    // Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();
#endif

    for(uint8_t t = 4; t > 0; t--) {
    #if DEBUG == 1
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
    #endif
        delay(1000);
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
        http.begin("http://door-display.groups.et.byu.net/30aea40b54c.compressed");

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
                            lastEntry = buff[0];
                            lastEntry = lastEntry % 2;
                            offset++;
                          }
                          counter = buff[offset];
                          if (counter == 255) {
                            for (int16_t i = cursor; i < cursor + 255; i++) {
                          #if DEBUG == 1
                              Serial.print(lastEntry);
                          #endif
                              if (i % X_RES == 0)
                            #if DEBUG == 1
                                Serial.println("");
                            #endif
                              display.drawPixel(i%X_RES, y+i/X_RES, lastEntry);
                            }
                            cursor += 255;
                          } else {
                            for (int16_t i = cursor; i < cursor + counter; i++) {
                          #if DEBUG == 1
                              Serial.print(lastEntry);
                          #endif
                              if (i % X_RES == 0)
                            #if DEBUG == 1
                                Serial.println("");
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

            #if DEBUG == 1
                Serial.println();
                Serial.print("[HTTP] connection closed or file end.\n");
            #endif

            }
        } else {
        #if DEBUG == 1
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        #endif
        }

        http.end();
    }

    delay(10000);
}

