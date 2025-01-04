#ifndef Structs_h
#define Structs_h

#include <Adafruit_GFX.h>
#include <config/Enums.h>

struct state_config {
    modes curr_mode = SCROLLING;
    screens curr_screen = MAIN;
    conn_statuses radio_status = NO_CONN;
    bool daytime = false;
    bool summertemp = false;
    bool setup = true;
    uint16_t sunset = 0;
    uint16_t sunrise = 0;
    uint8_t curr_mint = 0;
    uint8_t curr_weekday = 0;
    uint32_t prev_conn = 0;
};

struct indicator_config {
    const char* alignment;
    const uint16_t aln_x, aln_y;
    const uint16_t bound_x, bound_y;
    const uint16_t bound_width, bound_height;
    const uint16_t color;
    const GFXfont* font;
    const char* unit;
    const bool forced_round;
};

struct icon_config {
    const uint16_t* bitmap;
    const uint16_t x, y;
    const uint16_t width, height;
};

struct weathericon_config {
    const int8_t min_rating;
    const int8_t max_rating;
    const icon_config option1;
    const icon_config option2;
};

#endif