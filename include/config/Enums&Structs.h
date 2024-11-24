#ifndef enums_h
#define enums_h

#include <Adafruit_GFX.h>

enum modes {
    SCROLLING,
    PANNING,
    CURSOR
};

enum screens {
    MAIN,
    OUT_TEMP, OUT_HUM, OUT_PRESS,
    IN_TEMP, IN_HUM,
    CO2_RATE
};

enum conn_statuses {
    RECEIVING,
    PENDING,
    NO_CONN
};

struct indicator_config {
    const char* alignment;
    const uint16_t aln_x, aln_y;
    const uint16_t bound_x, bound_y;
    const uint16_t bound_width, bound_height;
    const uint16_t color;
    const GFXfont* font;
    const char* unit;
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