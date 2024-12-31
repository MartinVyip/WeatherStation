#ifndef Enums_h
#define Enums_h

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

enum periodics {
    POWER_TASK,
    ENC_TASK,
    RTC_TASK,
    PIR_TASK,
    BUFFERS_TASK,
    PERIODIC_BACKUP_TASK,
    APPEND_TASK,
    UPDATE_TASK,
    PLOT_TASK,
    NUM_TASKS
};

#endif