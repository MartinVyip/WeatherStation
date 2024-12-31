#ifndef SolarWeatherUtils_h
#define SolarWeatherUtils_h

#include <Arduino.h>
#include <utils/TimeUtils.h>
#include <config/Constants.h>

int8_t findWeatherRating(int8_t press_rate, int8_t hum_rate, int8_t temp_rate);
uint16_t toMmHg(float pascals);

bool checkDST(uint8_t month, uint8_t day, uint8_t weekday);
void adjustSolarEvents(state_config& state, uint8_t month, uint8_t day, uint8_t weekday);
void adjustSummertemp(bool* summertemp, float temp);
void adjustDaytime(state_config& state, uint8_t hour, uint8_t minute);

#endif