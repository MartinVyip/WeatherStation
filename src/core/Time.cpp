#include <config/Globals.h>
#include <core/Display.h>

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

void updateRTCEvents(state_config& state, uint8_t minute) {
    uint8_t month = rtc.getMonth(); uint8_t day = rtc.getDay();
    uint8_t weekday = rtc.getWeekDay();
    uint8_t hour = rtc.getHours();

    adjustDaytime(state, hour, minute);
    adjustDST(month, day, weekday, hour);
    updateTime(minute);

    if (weekday != state.curr_weekday) {
        state.curr_weekday = weekday;
        adjustSolarEvents(state, month, day, weekday);
        updateDate();
        updateIndicator(weekdays[weekday - 1], weekday_ind, false);
    }
}