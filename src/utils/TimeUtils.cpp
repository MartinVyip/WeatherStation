#include <utils/TimeUtils.h>

uint16_t findDayOfYear(uint8_t month, uint8_t day) {
    uint16_t months_sum = 0;
    for (uint8_t i = 0; i < month - 1; i++) {
        months_sum += days_in_month[i];
    }
    return months_sum + day;
}

uint16_t findMinutesOfDay(uint8_t hour, uint8_t min) {
    return hour * 60 + min;
}

uint16_t findDayMinutesDifference(uint16_t later_day_min, uint16_t earlier_day_min) {
    uint16_t min_diff = (later_day_min >= earlier_day_min)
                      ? later_day_min - earlier_day_min
                      : (24 * 60) - earlier_day_min + later_day_min;
    return min_diff;
}

uint32_t findYearMinutesDifference(uint16_t later_year_day, uint16_t later_day_min, 
                                   uint16_t earlier_year_day, uint16_t earlier_day_min) {

    uint16_t day_difference = (later_year_day >= earlier_year_day)
                            ? later_year_day - earlier_year_day
                            : 365 - earlier_year_day + later_year_day;
    uint16_t min_diff = later_day_min - earlier_day_min;
    return (day_difference * 24 * 60) + min_diff;
}