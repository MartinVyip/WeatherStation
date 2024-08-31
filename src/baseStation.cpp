#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <Constants.h>
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

enum modes {
    SCROLLING,
    PANNING,
    CURSOR
};

enum screens {
    MAIN,
    OUT_TEMP, OUT_HUM, OUT_PRESS,
    IN_PRESS, IN_HUM, IN_TEMP,
    CO2_RATE
};

inline void tftSetup() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    tft.begin();
    tft.setRotation(3);
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
    UART.begin(115200);
    I2C.begin();

    tftSetup();
    radioSetup();
    rtcSetup();
    co2Setup();
    eeprom.begin();
    bme.begin(0x76, &I2C);

    enc.setFastTimeout(ENC_FAST_TIME);
}

DataVault <float> out_temp(1680);
Graph <float> plot(out_temp, tft);

void setup() {
    hardwareSetup();

    const int numPoints = 1680;
    const float frequency = 10;

    int hour = 0;
    int minute = 0;

    for (int i = 0; i < numPoints; i++) {
        float angle = i * (2 * PI / numPoints);
        float value = sin(angle * frequency) * (numPoints - i);
        out_temp.appendValue(value, hour, minute);
        minute += 6;
        if (minute >= 60) {
            minute = 0;
            hour++;
            if (hour >= 24) {
                hour = 0;
            }
        }
    }

    plot.drawFresh(false);
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
    };

    switch(curr_mode) {
        case SCROLLING: {
            if (setup) {
                setup = false;
            }

            if (enc.left()) {
                curr_screen = (curr_screen > MAIN) ? (screens)(curr_screen - 1) : CO2_RATE;
                setup = true;
            }
            else if (enc.right()) {
                curr_screen = (curr_screen < CO2_RATE) ? (screens)(curr_screen + 1) : MAIN;
                setup = true;
            }

            if (enc.release()) {
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
            }

            if (enc.turn()) {
                enc.dir();
                enc.fast();
            }

            if (enc.release()) {
                curr_mode = CURSOR;
                setup = true;
            }
            if (enc.hold()) {
                while(enc.holding());
                curr_mode = SCROLLING;
                setup = true;
            }
        }
        break;
        case CURSOR: {
            if (setup) {
                setup = false;
            }

            if (enc.turn()) {
                enc.dir();
                enc.fast();
            }

            if (enc.release()) {
                curr_mode = PANNING;
                setup = true;
            }
            if (enc.hold()) {
                while(enc.holding());
                curr_mode = SCROLLING;
                setup = true;
            }
        }
        break;
    }
    if (enc.turn()) {
        int8_t step = ((enc.fast()) ? PAN_FAST : PAN_SLOW) * enc.dir();
        plot.dynamicPan(step);
    }
    //UART.printf("%02d.%02d.%02d  ", rtc.getDay(), rtc.getMonth(), rtc.getYear());
    //UART.printf("%02d:%02d:%02d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
}