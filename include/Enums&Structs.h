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
    uint16_t pos_x;
    uint16_t pos_y;
    uint16_t bound_x;
    uint16_t bound_y;
    uint16_t bound_width;
    uint16_t bound_height;
    const GFXfont* font;
    const char* unit;
};

#endif