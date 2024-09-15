#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <Constants.h>
#include <Enums.h>
#include <GraphingBase.h>
#include <GraphingEngine.h>
#include <DataVault.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_BME280.h>
#include <RF24.h>
#include <STM32RTC.h>
#include <EncButton.h>
#include <I2C_eeprom.h>
#include <MHZ19.h>

SPIClass tftSPI(TFT_MOSI, TFT_MISO, TFT_CLK);
HardwareSerial UART(RX, TX);
SoftwareSerial MH_UART(MH_RX, MH_TX);
TwoWire I2C(SDA, SCL);

Adafruit_ILI9341 tft(&tftSPI, TFT_DC, TFT_CS, TFT_RESET);
Adafruit_BME280 bme;
EncButton enc(ENC_S1, ENC_S2, ENC_KEY);
RF24 radio(RF_CE, RF_CSN);
MHZ19 co2;
I2C_eeprom eeprom(0x50, &I2C);
STM32RTC& rtc = STM32RTC::getInstance();

inline void tftSetup() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    tft.begin();
    tft.setRotation(3);
    tft.cp437(true);
}

inline void radioSetup() {
    radio.begin();
    radio.setAutoAck(true);
    radio.setRetries(0, 5);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(0x60);
    radio.openReadingPipe(1, 0x7878787878LL);
    radio.startListening();
}

inline void rtcSetup() {
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
}

inline void co2Setup() {
    MH_UART.begin(9600);
    co2.begin(MH_UART);
    co2.autoCalibration(false);
}

inline void hardwareSetup() {
    delay(500);
    UART.begin(115200);
    I2C.begin();

    co2Setup();
    radioSetup();
    rtcSetup();
    tftSetup();
    eeprom.begin();
    bme.begin(0x76, &I2C);

    enc.setFastTimeout(ENC_FAST_TIME);
}

DataVault <float> out_temp(1680);
DataVault <float> out_hum(1680);
DataVault <uint16_t> out_press(1680);
DataVault <float> in_temp(1680);
DataVault <float> in_hum(1680);
DataVault <uint16_t> co2_rate(1680);

GraphBase* plot = nullptr;

void setup() {
    hardwareSetup();

    const int numPoints = 1680;
    const float frequency = 10;

    int day = 0;
    int hour = 0;
    int minute = 0;

    for (int i = 0; i < numPoints; i++) {
        float angle = i * (2 * PI / numPoints);

        // Create unique value for out_temp using a mix of sine wave and sawtooth wave
        float value1 = sin(angle * frequency) * (numPoints - i) / 25 + (fmod(i, 100) / 5.0) - 10;
        out_temp.appendValue(value1, day, hour, minute);

        // out_hum with triangle wave
        value1 = abs(fmod(i * 2, 200) - 100) / 10.0 - 40 + cos(angle * frequency) * 5;
        out_hum.appendValue(value1, day, hour, minute);

        // in_temp with sine + square wave combination
        value1 = (sin(angle * frequency * 2) > 0 ? 1 : -1) * (numPoints - i) / 15 + sin(angle * frequency * 3) * 5 + 85;
        in_temp.appendValue(value1, day, hour, minute);

        // in_hum with a combination of triangle and sine wave
        value1 = (abs(fmod(i * 3, 300) - 150) / 15.0) + sin(angle * frequency / 4) * 5;
        in_hum.appendValue(value1, day, hour, minute);

        // out_press with sawtooth wave + sine wave
        uint16_t value2 = (fmod(i * 1.5, 300) / 1.5) + sin(angle * frequency * 3) * 15 + 990;
        out_press.appendValue(value2, day, hour, minute);

        // co2_rate with cosine + step function
        value2 = ((i % 200 > 100) ? 100 : 0) + cos(angle * frequency) * (numPoints - i) + 1950 + i % 50;
        co2_rate.appendValue(value2, day, hour, minute);
        minute += 6;
        if (minute >= 60) {
            minute = 0;
            hour++;
            if (hour >= 24) {
                hour = 0;
                day++;
                if (day > 7) {
                    day = 0;
                }
            }
        }
    }
}

void loop() {
    static modes curr_mode = SCROLLING;
    static screens curr_screen = MAIN;
    static bool setup = true;

    enc.tick();

    if (radio.available()) {
        float received_data[3];
        radio.read(&received_data, sizeof(received_data));
    }

    if (UART.available()) {
        uint8_t time_bytes[4];
        uint32_t unix_time;
        UART.readBytes(time_bytes, sizeof(time_bytes));
        unix_time = (uint32_t)time_bytes[0] << 24 |
                    (uint32_t)time_bytes[1] << 16 |
                    (uint32_t)time_bytes[2] << 8 |
                    (uint32_t)time_bytes[3];
        rtc.setEpoch(unix_time);
    }

    switch (curr_mode) {
        case SCROLLING: {
            if (setup) {
                setup = false;
                tft.fillScreen(0x0000);
                if (curr_screen != MAIN) {
                    switch (curr_screen) {
                        case OUT_TEMP: plot = new Graph<float>(out_temp, tft); break;
                        case OUT_HUM: plot = new Graph<float>(out_hum, tft); break;
                        case OUT_PRESS: plot = new Graph<uint16_t>(out_press, tft); break;
                        case IN_TEMP: plot = new Graph<float>(in_temp, tft); break;
                        case IN_HUM: plot = new Graph<float>(in_hum, tft); break;
                        case CO2_RATE: plot = new Graph<uint16_t>(co2_rate, tft); break;
                    }
                    plot->drawFresh();
                    plot->drawLogos(curr_screen, true);
                    plot->annotate();
                } else {
                    tft.setTextColor(TEXT_CLR0);
                    tft.setTextSize(1);
                    tft.setCursor(0, 0);
                    tft.printf("%02d.%02d.%02d  ", rtc.getDay(),
                                                   rtc.getMonth(),
                                                   rtc.getYear());
                    tft.setCursor(0, 10);
                    tft.printf("%02d:%02d:%02d\n", rtc.getHours(),
                                                   rtc.getMinutes(),
                                                   rtc.getSeconds());
                }
            }

            if (enc.turn()) {
                if (enc.left()) {
                    curr_screen = (curr_screen > MAIN) ? (screens)(curr_screen - 1) : CO2_RATE;
                } else if (enc.right()) {
                    curr_screen = (curr_screen < CO2_RATE) ? (screens)(curr_screen + 1) : MAIN;
                }
                delete plot;
                plot = nullptr;
                setup = true;
            } else if (enc.click()) {
                if (curr_screen != MAIN) {
                    curr_mode = PANNING;
                    setup = true;
                }
            }
        }
        break;
        case PANNING: {
            if (setup) {
                setup = false;
                plot->drawLocal(false);
                plot->drawLogos(curr_screen, true);
                plot->annotate();
            }

            if (enc.turn()) {
                int8_t step = ((enc.fast()) ? PAN_FAST : PAN_SLOW) * enc.dir();
                plot->dynamicPan(step);
            } else if (enc.click()) {
                curr_mode = CURSOR;
                setup = true;
            } else if (enc.hold()) {
                while(enc.holding());
                curr_mode = SCROLLING;
                plot->drawLocal();
                plot->drawLogos(curr_screen, true);
                plot->annotate();
            }
        }
        break;
        case CURSOR: {
            if (setup) {
                setup = false;
                plot->drawLocal();
                plot->drawLogos(curr_screen, true);
                plot->annotate(false);
                plot->drawCursor(true);
            }

            if (enc.turn()) {
                int8_t step = ((enc.fast()) ? CRSR_FAST : CRSR_SLOW) * enc.dir();
                plot->dynamicCursor(step);
            } else if (enc.click()) {
                curr_mode = PANNING;
                setup = true;
            } else if (enc.hold()) {
                while(enc.holding());
                curr_mode = SCROLLING;
                plot->drawLocal();
                plot->drawLogos(curr_screen, true);
                plot->annotate();
            }
        }
        break;
    }
}