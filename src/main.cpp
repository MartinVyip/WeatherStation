#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <rsc/Constants.h>
#include <GraphingBase.h>
#include <GraphingEngine.h>
#include <DataVault.h>
#include <SolarWeatherUtils.h>

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

DataVault <float> out_temp(DATA_PNTS_AMT, TEMP_NORM_RANGE);
DataVault <float> out_hum(DATA_PNTS_AMT, HUM_NORM_RANGE);
DataVault <uint16_t> out_press(DATA_PNTS_AMT, PRESS_NORM_RANGE);
DataVault <float> in_temp(DATA_PNTS_AMT);
DataVault <float> in_hum(DATA_PNTS_AMT);
DataVault <uint16_t> co2_rate(DATA_PNTS_AMT);
GraphBase* plot = nullptr;

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

template <typename input_type>
void updateIndicator(input_type value, const indicator_config& settings, bool initial) {
    char output[15];

    if constexpr (std::is_arithmetic<input_type>::value) {
        DataVault<input_type>::getCharValue(value, output);
        strcat(output, settings.unit);
    } else {
        strcpy(output, static_cast<const char*>(value));
    }

    tft.setTextColor(settings.color);
    tft.setFont(settings.font);

    if (settings.alignment == "left") {
        tft.setCursor(settings.aln_x, settings.aln_y);
    } else {
        uint16_t width = Graph<input_type>::getTextWidth(output, tft);
        if (settings.alignment == "right") {
            tft.setCursor(settings.aln_x - width, settings.aln_y);
        } else if (settings.alignment == "center") {
            tft.setCursor(settings.aln_x - (width >> 1), settings.aln_y);
        }
    }

    if (!initial) {
        tft.fillRect(settings.bound_x, settings.bound_y,
                     settings.bound_width, settings.bound_height, 0x0000);
    }

    tft.write(output);
}

void updateTime(uint8_t minute) {
    char timestring[5];
    sprintf(timestring, "%02d:%02d", rtc.getHours(), minute);
    updateIndicator(timestring, time_ind, false);
}

void updateDate() {
    char datestring[10];
    sprintf(datestring, "%02d.%02d.%d", rtc.getDay(), rtc.getMonth(), rtc.getYear());
    updateIndicator(datestring, date_ind, false);
}

void updateWeatherIcon(int8_t weather_rating, bool daytime, bool summertemp, bool initial) {
    if (!initial && (weather_rating >= positive_weathers[3].min_rating ||
        weather_rating <= negative_weathers[0].max_rating)) {
        tft.fillRect(weather_icon.x, weather_icon.y,
                     weather_icon.width, weather_icon.height, 0x0000);
    }

    if (weather_rating >= 0) {
        for (const auto& config : positive_weathers) {
            if (weather_rating >= config.min_rating && weather_rating <= config.max_rating) {
                const icon_config& optimal = daytime ? config.option1 : config.option2;
                tft.drawRGBBitmap(optimal.x, optimal.y, optimal.bitmap, optimal.width, optimal.height);
                break;
            }
        }
    } else {
        for (const auto& config : negative_weathers) {
            if (weather_rating >= config.min_rating && weather_rating <= config.max_rating) {
                const icon_config& optimal = summertemp ? config.option1 : config.option2;
                tft.drawRGBBitmap(optimal.x, optimal.y, optimal.bitmap, optimal.width, optimal.height);
                break;
            }
        }
    }
}

void updateConnectionIcon(conn_statuses connection_status, bool initial) {
    if (!initial) {
        tft.fillRect(link_icon.x, link_icon.y,
                     link_icon.width, link_icon.height, 0x0000);
    }
    const uint16_t* status_icon = (connection_status == RECEIVING) ? receiving
                                : (connection_status == NO_CONN) ? no_connect
                                : pending;
    tft.drawRGBBitmap(link_icon.x, link_icon.y, status_icon,
                      link_icon.width, link_icon.height);
}

void adjustDST(uint8_t month, uint8_t day, uint8_t weekday, uint8_t hour) {
    if (day >= 25 && weekday == 7) {
        if (month == 3 && hour == 3) {
            rtc.setHours(hour + 1);
        } else if (month == 10 && hour == 4) {
            rtc.setHours(hour - 1);
        }
    }
}

void adjustRTC() {
    uint8_t time_bytes[4];
    uint32_t unix_time;
    UART.readBytes(time_bytes, sizeof(time_bytes));
    unix_time = (uint32_t)time_bytes[0] << 24 |
                (uint32_t)time_bytes[1] << 16 |
                (uint32_t)time_bytes[2] << 8 |
                (uint32_t)time_bytes[3];
    rtc.setEpoch(unix_time);
}

inline void buildMainScreen(bool daytime, bool summertemp, conn_statuses connection_status) {
    tft.drawRGBBitmap(indoor_icon.x, indoor_icon.y,
                      indoor_ind, indoor_icon.width, indoor_icon.height);
    //updateIndicator(out_temp.getLastValue(), out_temp_ind, true);
    updateIndicator(-27.7, out_temp_ind, true);
    updateIndicator(out_hum.getLastValue(), out_hum_ind, true);
    updateIndicator(out_press.getLastValue(), out_press_ind, true);
    updateIndicator(bme.readTemperature(), in_temp_ind, true);
    updateIndicator(bme.readHumidity(), in_hum_ind, true);
    updateIndicator(co2.getCO2(), co2_rate_ind, true);
    updateIndicator(weekdays[rtc.getWeekDay() - 1], weekday_ind, true);
    int8_t rate = findWeatherRating(out_press.findNormalizedTrendSlope(BACKSTEP_PER),
                                    out_hum.findNormalizedTrendSlope(BACKSTEP_PER),
                                    out_temp.findNormalizedTrendSlope(BACKSTEP_PER)
                                    );
    updateWeatherIcon(rate, daytime, summertemp, true);
    updateConnectionIcon(connection_status, true);
    updateTime(rtc.getMinutes());
    updateDate();
}


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
        out_temp.appendToAverage(value1);
        out_temp.appendToVault(day, hour, minute);

        // out_hum with triangle wave
        value1 = abs(fmod(i * 2, 200) - 100) / 10.0 - 40 + cos(angle * frequency) * 5;
        out_hum.appendToAverage(value1);
        out_hum.appendToVault(day, hour, minute);

        // in_temp with sine + square wave combination
        value1 = (sin(angle * frequency * 2) > 0 ? 1 : -1) * (numPoints - i) / 15 + sin(angle * frequency * 3) * 5 + 85;
        in_temp.appendToAverage(value1);
        in_temp.appendToVault(day, hour, minute);

        // in_hum with a combination of triangle and sine wave
        value1 = (abs(fmod(i * 3, 300) - 150) / 15.0) + sin(angle * frequency / 4) * 5;
        in_hum.appendToAverage(value1);
        in_hum.appendToVault(day, hour, minute);

        // out_press with sawtooth wave + sine wave
        uint16_t value2 = sin(angle * frequency * 3) * 15 + 760;
        out_press.appendToAverage(value2);
        out_press.appendToVault(day, hour, minute);

        // co2_rate with cosine + step function
        value2 = ((i % 200 > 100) ? 100 : 0) + cos(angle * frequency) * (numPoints - i) + 1950 + i % 50;
        co2_rate.appendToAverage(value2);
        co2_rate.appendToVault(day, hour, minute);
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
    static conn_statuses sens_status = NO_CONN;
    static bool daytime, summertemp, setup = true;
    static uint16_t sunset, sunrise;
    static uint8_t curr_mint, curr_weekday;

    enc.tick();
    uint32_t curr_time = millis();
    static uint32_t prev_upd_sens = curr_time, prev_apd_sens = curr_time;
    static uint32_t prev_check = curr_time - TIME_CHECK_PER;
    static uint32_t prev_conn = curr_time - PENDING_THRES;

    if (curr_time - prev_apd_sens >= APD_PER) {
        prev_apd_sens = curr_time;
        uint8_t weekday = rtc.getWeekDay() - 1;
        uint8_t hour = rtc.getHours();
        uint8_t minute = rtc.getMinutes();

        out_temp.appendToVault(weekday, hour, minute);
        out_hum.appendToVault(weekday, hour, minute);
        out_press.appendToVault(weekday, hour, minute);
        in_temp.appendToVault(weekday, hour, minute);
        in_hum.appendToVault(weekday, hour, minute);
        co2_rate.appendToVault(weekday, hour, minute);

        if (curr_screen == MAIN) {
            int8_t rate = findWeatherRating(out_press.findNormalizedTrendSlope(BACKSTEP_PER),
                                            out_hum.findNormalizedTrendSlope(BACKSTEP_PER),
                                            out_temp.findNormalizedTrendSlope(BACKSTEP_PER)
                                            );
            updateWeatherIcon(rate, daytime, summertemp, false);
        }
    }

    if (UART.available()) {
        adjustRTC();
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
                    plot->drawLogos(curr_screen, summertemp);
                    plot->annotate();
                } else {
                    buildMainScreen(daytime, summertemp, sens_status);
                    prev_check = curr_time;
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
                prev_check = curr_time;
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
                plot->drawLogos(curr_screen, summertemp);
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
                plot->drawLogos(curr_screen, summertemp);
                plot->annotate();
            }
        }
        break;
        case CURSOR: {
            if (setup) {
                setup = false;
                plot->drawLocal();
                plot->drawLogos(curr_screen, summertemp);
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
                plot->drawLogos(curr_screen, summertemp);
                plot->annotate();
            }
        }
        break;
    }

    if (radio.available()) {
        float received_data[3];
        radio.read(&received_data, sizeof(received_data));
        out_temp.appendToAverage(received_data[0]);
        out_hum.appendToAverage(received_data[1]);
        out_press.appendToAverage(toMmHg(received_data[2]));

        prev_conn = curr_time;
        adjustSummertemp(&summertemp, received_data[0]);
        if (curr_screen == MAIN) {
            updateIndicator(received_data[0], out_temp_ind, false);
            updateIndicator(received_data[1], out_hum_ind, false);
            updateIndicator(toMmHg(received_data[2]), out_press_ind, false);
        }
    }

    if (curr_time - prev_upd_sens >= UPD_PER) {
        prev_upd_sens = curr_time;
        float temp = bme.readTemperature();
        float hum = bme.readHumidity();
        uint16_t ppm = co2.getCO2();

        in_temp.appendToAverage(temp);
        in_hum.appendToAverage(hum);
        co2_rate.appendToAverage(ppm);

        if (curr_screen == MAIN) {
            updateIndicator(temp, in_temp_ind, false);
            updateIndicator(hum, in_hum_ind, false);
            updateIndicator(ppm, co2_rate_ind, false);
        }
    }

    if (curr_screen == MAIN && curr_time - prev_check >= TIME_CHECK_PER) {
        uint8_t minute = rtc.getMinutes();
        if (minute != curr_mint) {
            curr_mint = minute;
            uint8_t month = rtc.getMonth(); uint8_t day = rtc.getDay();
            uint8_t weekday = rtc.getWeekDay();
            uint8_t hour = rtc.getHours();

            adjustDaytime(&daytime, sunrise, sunset, hour, minute);
            adjustDST(month, day, weekday, hour);
            updateTime(minute);

            if (weekday != curr_weekday) {
                curr_weekday = weekday;
                adjustSolarEvents(&sunrise, &sunset, month, day, weekday);
                updateDate();
                updateIndicator(weekdays[weekday - 1], weekday_ind, false);
            }
        }

        conn_statuses new_status = (curr_time - prev_conn < RECEIVE_THRES) ? RECEIVING
                                 : (curr_time - prev_conn >= PENDING_THRES) ? NO_CONN
                                 : PENDING;

        if (new_status != sens_status) {
            sens_status = new_status;
            if (curr_screen == MAIN) {
                updateConnectionIcon(sens_status, false);
            }
        }
    }
}