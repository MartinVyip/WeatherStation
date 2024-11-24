#ifndef TimeUtils_h
#define TimeUtils_h

#include <Arduino.h>
#include <config/Constants.h>

uint16_t findDayOfYear(uint8_t month, uint8_t day);
uint16_t findMinutesOfDay(uint8_t hour, uint8_t min);
uint16_t findDayMinutesDifference(uint16_t later_time, uint16_t earlier_time);
uint32_t findYearMinutesDifference(uint16_t later_year_day, uint16_t later_day_min, 
                                   uint16_t earlier_year_day, uint16_t earlier_day_min);

#endif