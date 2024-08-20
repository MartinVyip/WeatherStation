#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RF24.h>
#include <SPI.h>
#include <STM32RTC.h>
#include <EncButton.h>

#define CE 3
#define CSN 2

RF24 radio(CE, CSN);

float gauges[3];

void setup(void) {
    radio.begin();
    radio.setAutoAck(true);
    radio.setRetries(0, 5);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(0x60);
    radio.openReadingPipe(1, 0x7878787878LL);
    radio.startListening();
    Serial.begin(9600);
    Serial.println("start");
}

void loop(void) {
    if (radio.available()) {
        radio.read(&gauges, sizeof(gauges));
    Serial.print(gauges[0]);
    Serial.print(' ');
    Serial.print(gauges[1]);
    Serial.print(' ');
    Serial.println(gauges[2]);
    }
}