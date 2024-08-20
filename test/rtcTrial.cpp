#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RF24.h>
#include <SPI.h>
#include <STM32RTC.h>

#define TFT_CS 4
#define TFT_DC 5
#define TFT_RESET 6

STM32RTC& rtc = STM32RTC::getInstance();
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RESET);

void setup() {
    Serial.begin(9600);
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
}

void loop() {
    static uint32_t last_print;

    if (Serial.available()) {
        uint8_t time_bytes[4];
        uint32_t unix_time;
        Serial.readBytes(time_bytes, sizeof(time_bytes));
        unix_time = (uint32_t)time_bytes[0] << 24 |
                    (uint32_t)time_bytes[1] << 16 |
                    (uint32_t)time_bytes[2] << 8 |
                    (uint32_t)time_bytes[3];
        rtc.setEpoch(unix_time);
    }
    if (millis() - last_print > 2000) {
        tft.printf("%02d.%02d.%02d  ", rtc.getDay(), rtc.getMonth(), rtc.getYear());
        tft.printf("%02d:%02d:%02d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
        last_print = millis();
    }
}