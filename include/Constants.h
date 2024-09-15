#ifndef Constants_h
#define Constants_h

// ==================== SETTINGS ====================
#define PAN_SLOW 20
#define PAN_FAST 50
#define CRSR_SLOW 1
#define CRSR_FAST 10
#define TICK_PER 6

// ===================== COLORS =====================
#define TEXT_CLR0 0xFC18  // main screen
#define TEXT_CLR1 0xFC18  // ticks
#define TEXT_CLR2 0xFC18  // weekdays
#define TEXT_CLR3 0xFC18  // cursor
#define TEXT_CLR4 0xFC18  // annotations
#define LINK_CLR 0xFD20
#define AXIS_CLR 0x7BE0
#define TICK_CLR 0x7BE0
#define SEP_CLR 0x03EF
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

// =================== EXCEPTIONS ===================
#if (TICK_PER < 4 || 24 % TICK_PER != 0)
#error "Invalid ticking period"
#endif

#endif