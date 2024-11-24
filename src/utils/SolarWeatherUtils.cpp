#include <utils/SolarWeatherUtils.h>

int8_t findWeatherRating(int8_t press_rate, int8_t hum_rate, int8_t temp_rate) {
    int8_t rating = PRESS_WEIGHT * press_rate + HUM_WEIGHT * hum_rate + TEMP_WEIGHT * temp_rate;
    return constrain(rating, -100, 100);
}

uint16_t toMmHg(float pascals) {
    return round(pascals * 0.00750062);
}

bool checkDST(uint8_t month, uint8_t day, uint8_t weekday) {
    bool dst = (month > 3 && month < 10) ? true :
               (month < 3 || month > 10) ? false :
               (month == 3) ? ((day - weekday) >= 25 ? true : false) :
               (month == 10) ? ((day - weekday) >= 25 ? false : true) :
               false;

    return dst;
}

void adjustSolarEvents(uint16_t* sunrise, uint16_t* sunset,
                       uint8_t month, uint8_t day, uint8_t weekday) {
    uint16_t yearday = findDayOfYear(month, day);
    bool dst = checkDST(month, day, weekday);

    float aux = (float) (2 * PI / 365) * (yearday + 10);
    uint16_t daylight_minutes = (float) -DAYCHANGE_AMP * cos(aux) + 720;

    aux = (float) (2 * PI / 365) * (yearday >= 81) ? yearday - 81 : yearday + 284;
    int16_t eot = (float) 7.53 * cos(aux) + 1.5 * sin(aux) - 9.87 * sin(2 * aux);
    int16_t solar_noon = 720 + RAW_SOLAR_NOON + 60 * dst + eot;

    *sunrise = (solar_noon - (daylight_minutes >> 1));
    *sunset = (solar_noon + (daylight_minutes >> 1));
}

void adjustSummertemp(bool* summertemp, float temp) {
    *summertemp = (temp >= -1);
}

void adjustDaytime(bool* daytime,
                   uint16_t sunrise, uint16_t sunset, uint8_t hour, uint8_t minute) {
    uint16_t curr_minutes = hour * 60 + minute;
    *daytime = (curr_minutes >= sunrise && curr_minutes < sunset);
}