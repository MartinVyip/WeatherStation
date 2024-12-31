#ifndef Backup_h
#define Backup_h

#include <Arduino.h>

void saveInt(uint16_t value, uint16_t* addr);
uint16_t readInt(uint16_t* addr);

void createRawBackup();
void finalizeBackup();

void pullBackup();
void restoreAuxiliaryData(uint16_t &addr, uint32_t &elapsed_time, uint16_t &periodic_cnt,
                          uint8_t &emergency_cnt, uint16_t &missing_cnt, uint16_t &start_index);

#endif