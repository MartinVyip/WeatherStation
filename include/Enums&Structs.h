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

struct indicator {
    const char* alignment;
    const uint16_t aln_x;
    const uint16_t aln_y;
    const uint16_t bound_x;
    const uint16_t bound_y;
    const uint16_t bound_width;
    const uint16_t bound_height;
    const uint16_t color;
    const GFXfont* font;
    const char* unit;
};

#endif