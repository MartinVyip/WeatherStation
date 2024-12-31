#include <config/Globals.h>

void tftSetup() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    tft.begin();
    tft.setRotation(3);
    tft.cp437(true);
}

void radioSetup() {
    radio.begin();
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(0x60);
    radio.openReadingPipe(1, 0x7878787878LL);
    radio.startListening();
}

void rtcSetup() {
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
}

void eepromSetup() {
    eeprom.begin();
    eeprom.setPageSize(64);
}

void hardwareSetup() {
    delay(1000);
    pinMode(LED, OUTPUT);
    pinMode(POW, INPUT); pinMode(PIR, INPUT_PULLUP);
    UART.begin(115200); I2C.begin(); bme.begin();
    radioSetup(); rtcSetup(); eepromSetup(); tftSetup();
    enc.setFastTimeout(ENC_FAST_TIME);
}