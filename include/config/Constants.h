#ifndef Constants_h
#define Constants_h

#include <rsc/fonts/CustomFont10pt.h>
#include <rsc/fonts/CustomFont12pt.h>
#include <rsc/fonts/CustomFont18pt.h>
#include <rsc/fonts/CustomFont24pt.h>
#include <rsc/Bitmaps.h>
#include <config/Enums.h>
#include <config/Structs.h>


// ==================== SETTINGS ====================

#define DATA_PNTS_AMT 1200  // amount of stored data points
#define UPD_PER 60000  // indoor sensors polling period [ms]
#define APD_PER 360000  // period of appending new values to vault [ms]
#define AWAKE_PER 60000  // display backlight timeout [ms]
#define PAN_SLOW 20  // panning speed slow [data points/turn]
#define PAN_FAST 50  // panning speed fast [data points/turn]
#define CRSR_SLOW 1  // cursor speed slow [data points/turn]
#define CRSR_FAST 10  // cursor speed fast [data points/turn]
#define TICK_PER 6  // graph ticks period [hours]

#define BACKSTEP_PER 90  // time period used for weather prediction [min]
#define PRESS_NORM_RANGE 0.01  // highest pressure change [mmHg/min]
#define HUM_NORM_RANGE 0.5  // highest humidity change [%/min]
#define TEMP_NORM_RANGE 0.15  // highest temperature change [°C/min]

#define LONGITUDE 24.75  // station location longitude [degrees]
#define LONGEST_DAY 18  // day length during summer solstice [hours]
#define SHORTEST_DAY 6  // day lenght during winter solstice [hours]
#define GMT_OFFSET 2  // GMT offset, without Daylight Saving Time (DST) [hours]


// ===================== COLORS =====================

#define TEXT_CLR1 0xFE5C  // graph tick data
#define TEXT_CLR2 0xFE5C  // graph weekday
#define TEXT_CLR3 0xFE5C  // cursor data
#define TEXT_CLR4 0xFE5C  // graph annotation data
#define LINK_CLR 0xFD20  // graph annotation arrow
#define AXIS_CLR 0x7BE0  // graph axis
#define TICK_CLR 0x7BE0  // graph tick line
#define SEP_CLR 0x7BE0  // graph weekdays separator line
#define PLOT_CLR 0xC618  // curve
#define CRSR_CLR 0xF800  // cursor


// ===================== IO PINS ====================

#define LED PC6
#define TX PA9
#define RX PA10

#define SDA PB9
#define SCL PB8

#define TFT_MOSI PB15
#define TFT_MISO PB14  // NC
#define TFT_CLK PB13
#define TFT_CS PD2
#define TFT_DC PC11
#define TFT_RESET PC12
#define TFT_LED PC10

#define RF_CE PC4
#define RF_CSN PA4

#define MH_PWM PA14
#define MH_HD PA15

#define PIR PC1
#define POW PC5

#define ENC_KEY PA3
#define ENC_S1 PA1
#define ENC_S2 PA2


// ============== DEVELOPMENT CONSTANTS =============

#define RAW_SOLAR_NOON (60 * (GMT_OFFSET - LONGITUDE / 15))
#define DAYCHANGE_AMP (60 * ((LONGEST_DAY - SHORTEST_DAY) >> 1))

#define PRESS_WEIGHT 0.6
#define HUM_WEIGHT 0.3
#define TEMP_WEIGHT 0.1

#define RECEIVE_THRES 1000
#define PENDING_THRES 300000

#define STORE_PER 3600000
#define POLL_POW_PER 10
#define POLL_ENC_PER 5
#define POLL_BUFS_PER 50
#define POLL_PIR_PER 500
#define POLL_RTC_PER 250
#define SCREEN_UPD_PER 50

#define ENC_FAST_TIME 150
#define APD_PER_S (APD_PER / 60000)
#define BYTES_PER_HOUR ((STORE_PER / APD_PER) << 1)

#define TFT_XMAX 320
#define TFT_YMAX 240
#define L_EDGE 80
#define UP_EDGE 50
#define BT_EDGE 220
#define TICK_LEN 10
#define SEP_LEN 35
#define CRECT_SIDE 16
#define CRECT_HALF (CRECT_SIDE >> 1)

const char degree_celcius[] = {0x7F, 'C', '\0'};
const indicator_config out_temp_ind = {"right", 310, 40, 104, 4, 207, 37, 0xFE5C,
                                       &CustomFont24pt, degree_celcius, false};
const indicator_config out_hum_ind = {"right", 310, 77, 169, 47, 142, 31, 0x7BFF,
                                      &CustomFont18pt, " %", false};
const indicator_config out_press_ind = {"right", 310, 100, 178, 86, 132, 15, 0x2D6A,
                                        &CustomFont10pt, " mmHg", true};
const indicator_config in_temp_ind = {"center", 250, 180, 192, 161, 116, 20, 0xFE5C,
                                      &CustomFont12pt, degree_celcius, false};
const indicator_config in_hum_ind = {"center", 250, 206, 203, 186, 95, 22, 0x7BFF,
                                     &CustomFont12pt, " %", false};
const indicator_config co2_rate_ind = {"center", 250, 230, 186, 216, 129, 15, 0x2D6A,
                                       &CustomFont10pt, " PPM", false};
const indicator_config time_ind = {"center", 90, 178, 5, 144, 172, 35, 0xFE5C,
                                   &CustomFont24pt, "", false};
const indicator_config weekday_ind = {"center", 90, 208, 10, 192, 161, 17, 0xFE5C,
                                      &CustomFont10pt, "", false};
const indicator_config date_ind = {"center", 90, 230, 36, 216, 109, 15, 0xFE5C,
                                   &CustomFont10pt, "", false};

const uint16_t* const summer_graph_icons[] PROGMEM = {
    nullptr,
    high_temp_out, hum_out, press_out,
    high_temp_in, hum_in, co2_in
};

const uint16_t* const winter_graph_icons[] PROGMEM = {
    nullptr,
    low_temp_out, hum_out, press_out,
    low_temp_in, hum_in, co2_in
};

const icon_config tech_icon = {tal_tech, 10, 130, 50, 30};
const icon_config indoor_icon = {indoor_ind, 190, 120, 120, 40};

const icon_config link_icon = {nullptr, 10, 5, 40, 35};
const icon_config graph_icon = {nullptr, 5, 5, 60, 60};
const icon_config weather_icon = {nullptr, 28, 51, 115, 75};

const weathericon_config positive_weathers[] = {
    {80, 100, {clear_day, 48, 51, 75, 75}, {clear_night, 56, 60, 60, 60}},
    {60, 79, {sunny_day, 31, 51, 110, 75}, {starry_night, 31, 51, 110, 75}},
    {40, 59, {good_day, 41, 51, 90, 75}, {good_night, 36, 51, 100, 75}},
    {20, 39, {cloudy_day, 41, 51, 90, 75}, {cloudy_night, 41, 56, 90, 65}}
};

const weathericon_config negative_weathers[] = {
    {-36, -20, {moody_clouds, 40, 62, 92, 56}, {moody_clouds, 40, 62, 92, 56}},
    {-53, -37, {sudden_rain, 38, 51, 95, 75}, {sudden_snow, 38, 54, 95, 70}},
    {-69, -54, {decent_rain, 41, 51, 90, 75}, {decent_snow, 38, 54, 95, 70}},
    {-85, -70, {shower, 41, 51, 90, 75}, {blizzard, 38, 54, 95, 70}},
    {-100, -86, {thunderstorm, 41, 51, 90, 75}, {blizzard, 38, 54, 95, 70}}
};

const char weekdays[7][12] = {
    {'E', 's', 'm', 'a', 's', 'p', 0x80, 'e', 'v', '\0'},
    {'T', 'e', 'i', 's', 'i', 'p', 0x80, 'e', 'v', '\0'},
    {'K', 'o', 'l', 'm', 'a', 'p', 0x80, 'e', 'v', '\0'},
    {'N', 'e', 'l', 'j', 'a', 'p', 0x80, 'e', 'v', '\0'},
    {'R', 'e', 'e', 'd', 'e', '\0'},
    {'L', 'a', 'u', 'p', 0x80, 'e', 'v', '\0'},
    {'P', 0x81, 'h', 'a', 'p', 0x80, 'e', 'v', '\0'}
};

const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


// ===================== MACROS =====================

#define SAVE_BACKUP_STATE(ready) (eeprom.writeByte(0, (ready)))
#define READ_BACKUP_STATE()      (eeprom.readByte(0))
#define STORE_BYTES(array, index, bytes) \
    (array)[(index) << 1] = (bytes)[0]; \
    (array)[((index) << 1) + 1] = (bytes)[1];


// =================== EXCEPTIONS ===================

#if (TICK_PER < 4 || 24 % TICK_PER != 0)
#error "Invalid ticking period"
#endif

#if (STORE_PER % APD_PER != 0)
#error "Only whole number of appends should fit into storage period"
#endif

#if (DATA_PNTS_AMT > 1200)
#warning "Stack overflow risk due to statically filled RAM"
#endif

#endif