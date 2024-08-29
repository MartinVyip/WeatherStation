#ifndef Constants_h
#define Constants_h

// ==================== SETTINGS ====================
#define TEXT_COLOR 0xC618
#define AXIS_COLOR 0x07E0
#define TICK_COLOR 0x07E0
#define PLOT_COLOR 0xFFFF
#define TICK_LEN 10
#define TICK_PER 6

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
#define TFT_XMAX 320
#define TFT_YMAX 240
#define L_EDGE 80
#define UP_EDGE 50
#define BT_EDGE 220

// =================== EXCEPTIONS ===================
#if (TICK_PER < 4 || 24 % TICK_PER != 0)
#error "Invalid ticking period"
#endif

#if (TICK_LEN <= 0 || TICK_LEN > TFT_YMAX - BT_EDGE)
#error "Invalid tick length"
#endif

#endif