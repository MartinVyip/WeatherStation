#ifndef Time_h
#define Time_h

#include <Arduino.h>

void adjustDST(uint8_t month, uint8_t day, uint8_t weekday, uint8_t hour);
void adjustRTC();

void updateRTCEvents(state_config& state, uint8_t minute);

#endif