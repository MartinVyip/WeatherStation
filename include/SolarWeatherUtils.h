#ifndef SolarWeatherUtils_h
#define SolarWeatherUtils_h

#include <Arduino.h>
#include <rsc/Constants.h>

int8_t findWeatherRating(int8_t press_rate, int8_t hum_rate, int8_t temp_rate);
uint16_t toMmHg(float pascals);

uint16_t findDayOfYear(uint8_t month, uint8_t day);
bool checkDST(uint8_t month, uint8_t day, uint8_t weekday);
void adjustSolarEvents(uint16_t* sunrise, uint16_t* sunset,
                       uint8_t month, uint8_t day, uint8_t weekday);
void adjustSummertemp(bool* summertemp, float temp);
void adjustDaytime(bool* daytime, uint16_t sunrise, uint16_t sunset,
                   uint8_t hour, uint8_t minute);

#endif