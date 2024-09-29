#ifndef Constants_h
#define Constants_h

#include <fonts/CustomFont10pt.h>
#include <fonts/CustomFont12pt.h>
#include <fonts/CustomFont18pt.h>
#include <fonts/CustomFont24pt.h>
#include <Enums&Structs.h>

// ==================== SETTINGS ====================
#define UPD_PER 30000
#define APD_PER 360000
#define PAN_SLOW 20
#define PAN_FAST 50
#define CRSR_SLOW 1
#define CRSR_FAST 10
#define TICK_PER 6

// ===================== COLORS =====================
#define TEXT_CLR1 0xFE5C  // ticks
#define TEXT_CLR2 0xFE5C  // weekdays
#define TEXT_CLR3 0xFE5C  // cursor
#define TEXT_CLR4 0xFE5C  // annotations
#define LINK_CLR 0xFD20
#define AXIS_CLR 0x7BE0
#define TICK_CLR 0x7BE0
#define SEP_CLR 0x7BE0
#define PLOT_CLR 0xC618
#define CRSR_CLR 0xF800

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

#define MH_TX PA14
#define MH_RX PA15

#define PIR PC1

#define ENC_KEY PA3
#define ENC_S1 PA1
#define ENC_S2 PA2

// ===============DEVELOPMENT CONSTANTS =============
#define CHECK_PER 250
#define ENC_FAST_TIME 150

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
const indicator out_temp_ind = {"right", 310, 40, 110, 4, 200, 36, 0xFE5C,
                                &CustomFont24pt, degree_celcius};
const indicator out_hum_ind = {"right", 310, 77, 173, 47, 137, 30, 0x7BFF,
                               &CustomFont18pt, " %"};
const indicator out_press_ind = {"right", 310, 100, 178, 86, 132, 14, 0x2D6A,
                                 &CustomFont10pt, " mmHg"};
const indicator in_temp_ind = {"center", 252, 180, 203, 161, 98, 19, 0xFE5C,
                               &CustomFont12pt, degree_celcius};
const indicator in_hum_ind = {"center", 252, 205, 205, 185, 93, 20, 0x7BFF,
                              &CustomFont12pt, " %"};
const indicator co2_rate_ind = {"center", 252, 230, 188, 216, 128, 14, 0x2D6A,
                                &CustomFont10pt, " PPM"};
const indicator time_ind = {"center", 90, 180, 5, 146, 170, 35, 0xFE5C,
                            &CustomFont24pt, ""};
const indicator weekday_ind = {"center", 90, 210, 0, 0, 0, 0, 0xFE5C,
                            &CustomFont10pt, ""};
const indicator date_ind = {"center", 90, 232, 0, 0, 0, 0, 0xFE5C,
                            &CustomFont10pt, ""};

const char weekdays[7][12] = {
    {'E', 's', 'm', 'a', 's', 'p', 0x80, 'e', 'v', '\0'},
    {'T', 'e', 'i', 's', 'i', 'p', 0x80, 'e', 'v', '\0'},
    {'K', 'o', 'l', 'm', 'a', 'p', 0x80, 'e', 'v', '\0'},
    {'N', 'e', 'l', 'j', 'a', 'p', 0x80, 'e', 'v', '\0'},
    {'R', 'e', 'e', 'd', 'e', '\0'},
    {'L', 'a', 'u', 'p', 0x80, 'e', 'v', '\0'},
    {'P', 0x81, 'h', 'a', 'p', 0x80, 'e', 'v', '\0'}
};

// =================== EXCEPTIONS ===================
#if (TICK_PER < 4 || 24 % TICK_PER != 0)
#error "Invalid ticking period"
#endif

#endif