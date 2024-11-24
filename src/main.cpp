#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <config/Constants.h>
#include <classes/GraphingEngine.h>
#include <classes/DataVault.h>
#include <utils/SolarWeatherUtils.h>
#include <utils/TimeUtils.h>

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
I2C_eeprom eeprom(0x50, I2C_DEVICESIZE_24LC256, &I2C);
STM32RTC& rtc = STM32RTC::getInstance();

DataVault <float> out_temp(DATA_PNTS_AMT, TEMP_NORM_RANGE, eeprom);
DataVault <float> out_hum(DATA_PNTS_AMT, HUM_NORM_RANGE, eeprom);
DataVault <uint16_t> out_press(DATA_PNTS_AMT, PRESS_NORM_RANGE, eeprom);
DataVault <float> in_temp(DATA_PNTS_AMT, eeprom);
DataVault <float> in_hum(DATA_PNTS_AMT, eeprom);
DataVault <uint16_t> co2_rate(DATA_PNTS_AMT, eeprom);

GraphBase* plot = nullptr;
std::vector<VaultBase*> vaults = {
    &out_temp, &out_hum, &out_press,
    &in_temp, &in_hum,
    &co2_rate
};

uint16_t last_day_min;
bool backup_ready = false;
volatile uint16_t year_day = 0;
volatile uint16_t day_min = 0;
volatile bool backup_flag = false;

inline void tftSetup() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    tft.begin();
    tft.setRotation(3);
    tft.cp437(true);
}

inline void radioSetup() {
    radio.begin();
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

inline void eepromSetup() {
    eeprom.begin();
    eeprom.setPageSize(64);
}

inline void co2Setup() {
    MH_UART.begin(9600);
    //co2.begin(MH_UART);
    //co2.autoCalibration(false);
}

inline void hardwareSetup() {
    delay(500);
    UART.begin(115200); I2C.begin();
    co2Setup(); radioSetup(); rtcSetup(); tftSetup(); eepromSetup();
    bme.begin(0x76, &I2C);
    enc.setFastTimeout(ENC_FAST_TIME);
    pinMode(LED, OUTPUT); pinMode(POW, INPUT); pinMode(PIR, INPUT);
}

void saveInt(uint16_t value, uint16_t* addr) {
    eeprom.writeByte((*addr)++, (value >> 8) & 0xFF);
    eeprom.writeByte((*addr)++, value & 0xFF);
}

uint16_t readInt(uint16_t* addr) {
    uint8_t high_byte = eeprom.readByte((*addr)++);
    uint8_t low_byte = eeprom.readByte((*addr)++);
    return (high_byte << 8) | low_byte;
}

void createRawBackup() {
    uint16_t addr = 6;

    SAVE_BACKUP_STATE(false);
    saveInt(out_temp.getHeadCount(), &addr);
    for (auto& vault : vaults) {
        vault->savePeriodicData(&addr);
    }
    last_day_min = findMinutesOfDay(rtc.getHours(), rtc.getMinutes());
    backup_ready = true;
}

void finalizeBackup() {
    uint8_t elapsed_time = findDayMinutesDifference(day_min, last_day_min);
    uint8_t emergency_data_count = backup_ready ? (elapsed_time / APD_PER_S) : 0;

    uint16_t addr = 1;
    saveInt(year_day, &addr);
    saveInt(day_min, &addr);
    eeprom.writeByte(addr++, emergency_data_count);
    if (emergency_data_count) {
        for (auto& vault : vaults) {
            vault->saveEmergencyData(emergency_data_count);
        }
    } else if (!backup_ready) {
        saveInt(out_temp.getHeadCount(), &addr);
        for (auto& vault : vaults) {
            vault->savePeriodicData(&addr);
        }
    }
    SAVE_BACKUP_STATE(true);
}

void rawBackupTask(void*) {
    createRawBackup();
    vTaskDelete(NULL);
}

void finalizeBackupInterrupt() {
    digitalWrite(TFT_LED, LOW);
    year_day = findDayOfYear(rtc.getMonth(), rtc.getDay());
    day_min = findMinutesOfDay(rtc.getHours(), rtc.getMinutes());
    backup_flag = true;
}

void restoreAuxiliaryData(uint16_t &addr, uint32_t &elapsed_time, uint16_t &periodic_cnt,
                          uint8_t &emergency_cnt, uint16_t &missing_cnt, uint16_t &start_index) {
    uint16_t curr_year_day = findDayOfYear(rtc.getMonth(), rtc.getDay());
    uint16_t curr_day_min = findMinutesOfDay(rtc.getHours(), rtc.getMinutes());
    elapsed_time = findYearMinutesDifference(curr_year_day, curr_day_min,
                                             readInt(&addr), readInt(&addr));

    emergency_cnt = eeprom.readByte(addr++);
    periodic_cnt = readInt(&addr);
    missing_cnt = elapsed_time / APD_PER_S;
    uint16_t total_count = periodic_cnt + emergency_cnt + missing_cnt;
    start_index = (total_count > DATA_PNTS_AMT) ? total_count - DATA_PNTS_AMT : 0;
}

void pullBackup() {
    uint16_t addr = 1;
    uint32_t elapsed_time;
    uint16_t periodic_cnt, missing_cnt, start_idx;
    uint8_t emergency_cnt;
    uint8_t curr_wday = rtc.getWeekDay(), curr_hour = rtc.getHours(), curr_min = rtc.getMinutes();

    restoreAuxiliaryData(addr, elapsed_time, periodic_cnt, emergency_cnt, missing_cnt, start_idx);
    if (elapsed_time < DATA_PNTS_AMT * APD_PER_S) {
        for (auto& vault : vaults) {
            vault->restorePointsData(&addr, start_idx, periodic_cnt, emergency_cnt, missing_cnt);
            vault->assignTimestamps(curr_wday, curr_hour, curr_min);
        }
    }
}

void clearRectangle(const icon_config& icon) {
    tft.fillRect(icon.x, icon.y, icon.width, icon.height, 0x0000);
}

void drawIcon(const icon_config& icon) {
    tft.drawRGBBitmap(icon.x, icon.y, icon.bitmap, icon.width, icon.height);
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
    static const icon_config* optimal = nullptr;

    if (!initial && (weather_rating >= positive_weathers[3].min_rating ||
        weather_rating <= negative_weathers[0].max_rating)) {
        clearRectangle(weather_icon);
    } else if (initial && (weather_rating < positive_weathers[3].min_rating ||
               weather_rating > negative_weathers[0].max_rating) && optimal != nullptr) {
        drawIcon(*optimal);
    }

    if (weather_rating >= 0) {
        for (const auto& config : positive_weathers) {
            if (weather_rating >= config.min_rating && weather_rating <= config.max_rating) {
                optimal = daytime ? &config.option1 : &config.option2;
                drawIcon(*optimal);
                break;
            }
        }
    } else {
        for (const auto& config : negative_weathers) {
            if (weather_rating >= config.min_rating && weather_rating <= config.max_rating) {
                optimal = summertemp ? &config.option1 : &config.option2;
                drawIcon(*optimal);
                break;
            }
        }
    }
}

void updateConnectionIcon(enum conn_statuses connection_status, bool initial) {
    if (!initial) clearRectangle(link_icon);
    const uint16_t* status_bitmap = (connection_status == RECEIVING) ? receiving
                                  : (connection_status == NO_CONN) ? no_connect
                                  : pending;
    icon_config status_icon = {status_bitmap, link_icon.x, link_icon.y,
                               link_icon.width, link_icon.height};
    drawIcon(status_icon);
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

inline void buildMainScreen(bool daytime, bool summertemp, enum conn_statuses connection_status) {
    drawIcon(indoor_icon);
    updateIndicator(out_temp.getLastValue(), out_temp_ind, true);
    updateIndicator(out_hum.getLastValue(), out_hum_ind, true);
    updateIndicator(out_press.getLastValue(), out_press_ind, true);
    updateIndicator(bme.readTemperature(), in_temp_ind, true);
    updateIndicator(bme.readHumidity(), in_hum_ind, true);
    //updateIndicator(co2.getCO2(), co2_rate_ind, true);
    updateIndicator(weekdays[rtc.getWeekDay() - 1], weekday_ind, true);

    int8_t rate = findWeatherRating(out_press.findNormalizedTrendSlope(BACKSTEP_PER),
                                    out_hum.findNormalizedTrendSlope(BACKSTEP_PER),
                                    out_temp.findNormalizedTrendSlope(BACKSTEP_PER));
    updateWeatherIcon(rate, daytime, summertemp, true);
    updateConnectionIcon(connection_status, true);
    updateTime(rtc.getMinutes());
    updateDate();
}


void setup() {
    hardwareSetup();
    attachInterrupt(digitalPinToInterrupt(POW), finalizeBackupInterrupt, FALLING);

    if (READ_BACKUP_STATE()) {
        tft.fillScreen(0x0000);
        pullBackup();
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
    static uint32_t prev_backup = curr_time, prev_pir_signal = curr_time;
    static uint32_t prev_check = curr_time - TIME_CHECK_PER;
    static uint32_t prev_conn = curr_time - PENDING_THRES;

    if (backup_flag) {
        noInterrupts();
        finalizeBackup();
        backup_flag = false;
    }

    if (curr_time - prev_apd_sens >= APD_PER) {
        prev_apd_sens = curr_time;
        uint8_t weekday = rtc.getWeekDay() - 1;
        uint8_t hour = rtc.getHours();
        uint8_t minute = rtc.getMinutes();

        for (auto& vault : vaults) {
            vault->appendToVault(weekday, hour, minute);
        }

        if (curr_screen == MAIN) {
            int8_t rate = findWeatherRating(out_press.findNormalizedTrendSlope(BACKSTEP_PER),
                                            out_hum.findNormalizedTrendSlope(BACKSTEP_PER),
                                            out_temp.findNormalizedTrendSlope(BACKSTEP_PER));
            updateWeatherIcon(rate, daytime, summertemp, false);
        }
    } else if (curr_time - prev_backup >= STORE_PER) {
        prev_backup = curr_time;
        xTaskCreate(rawBackupTask, "periodic_backup", 1024, NULL, 2, NULL);
    }

    if (UART.available()) {
        prev_check -= TIME_CHECK_PER;
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

            if (enc.turn() && out_temp.getHeadCount()) {
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
                if (curr_screen != MAIN && out_temp.getHeadCount() > TFT_XMAX - L_EDGE) {
                    curr_mode = PANNING;
                    setup = true;
                } else if (curr_screen != MAIN && out_temp.getHeadCount() > CRECT_SIDE) {
                    curr_mode = CURSOR;
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
            } else if (enc.click() && out_temp.getHeadCount() > TFT_XMAX - L_EDGE) {
                curr_mode = PANNING;
                setup = true;
            } else if (enc.hold() || enc.click()) {
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
        //uint16_t ppm = co2.getCO2();

        in_temp.appendToAverage(temp);
        in_hum.appendToAverage(hum);
        //co2_rate.appendToAverage(ppm);

        if (curr_screen == MAIN) {
            updateIndicator(temp, in_temp_ind, false);
            updateIndicator(hum, in_hum_ind, false);
            //updateIndicator(ppm, co2_rate_ind, false);
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

    if (digitalRead(TFT_LED) && curr_time - prev_pir_signal > AWAKE_PER) {
        digitalWrite(TFT_LED, LOW);
        if (curr_screen != MAIN) {
            curr_screen = MAIN;
            curr_mode = SCROLLING;
            setup = true;
        }
    } else if (digitalRead(PIR)) {
        digitalWrite(TFT_LED, HIGH);
        prev_pir_signal = curr_time;
    }
}