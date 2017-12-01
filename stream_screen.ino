#define USE_SERIAL Serial

#include <GxEPD.h>
#include <GxGDEW075T8/GxGDEW075T8.cpp>      // 7.5" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

GxIO_Class io(SPI, 15, 0, 2);
GxEPD_Class display(io, 2, 12);
#define X_RES 384
#define Y_RES 640

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;

void setup() {

    USE_SERIAL.begin(115200);
    // USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP("BYUSecure", "byuwireless");

}

void loop() {
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {

        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");

        // configure server and url
        http.begin("http://door-display.groups.et.byu.net/30aea40b54b.compressed");
        //http.begin("192.168.1.12", 80, "/test.html");

        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                display.init();
                display.setRotation(1);
                int cursor = 0;
                uint8_t prevColor = 0;
                uint8_t lastEntry;
                int16_t counter = 0;
                int16_t y = 0;
                // get lenght of document (is -1 when Server sends no Content-Length header)
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
                        // write it to Serial
                        //USE_SERIAL.write(buff, c);
                        for (int offset = 0; offset < c; offset++) {
                          if (cursor == 0 && offset == 0) {
                            lastEntry = buff[0];
                            offset++;
                          }
                          counter = buff[offset];
                          if (counter == 255) {
                            for (int16_t i = cursor; i < cursor + 255; i++) {
                              //Serial.print(i%X_RES);
                              //Serial.print(",");
                              //Serial.println(y+i/X_RES);
                              display.drawPixel(i%X_RES, y+i/X_RES, lastEntry);
                            }
                            cursor += 255;
                          } else {
                            for (int16_t i = cursor; i < cursor + counter; i++) {
                              //Serial.print(i%X_RES);
                              //Serial.print(",");
                              //Serial.println(y+i/X_RES);
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
                        //cursor += c;
                        if(len > 0) {
                            len -= c;
                        }
                    }
                    delay(1);
                }
                display.update();

                USE_SERIAL.println();
                USE_SERIAL.print("[HTTP] connection closed or file end.\n");

            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }

    delay(10000);
}

