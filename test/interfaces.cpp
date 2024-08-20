#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RF24.h>
#include <SPI.h>
#include <STM32RTC.h>
#include <EncButton.h>

#define LED PC6
#define TX PA9
#define RX PA10
#define SDA PB9
#define SCL PB8

HardwareSerial debug(RX, TX);
TwoWire I2C(SDA, SCL);

void setup() {
    debug.begin(115200);
    I2C.begin();
}

void loop() {
    
}