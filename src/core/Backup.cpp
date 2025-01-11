#include <config/Globals.h>

void saveInt(uint16_t value, uint16_t* addr) {
    eeprom.writeByte((*addr)++, (value >> 8) & 0xFF);
    eeprom.writeByte((*addr)++, value & 0xFF);
}

uint16_t readInt(uint16_t* addr) {
    uint8_t high_byte = eeprom.readByte((*addr)++);
    uint8_t low_byte = eeprom.readByte((*addr)++);
    return (high_byte << 8) | low_byte;
}

void createRawBackup() {
    uint16_t addr = 6;

    SAVE_BACKUP_STATE(false);
    saveInt(out_temp.getHeadCount(), &addr);
    for (auto& vault : vaults) {
        vault->savePeriodicData(&addr);
    }
    last_day_min = findMinutesOfDay(rtc.getHours(), rtc.getMinutes());
    backup_ready = true;
}

void finalizeBackup() {
    if (READ_BACKUP_STATE()) return;
    uint16_t year_day = findDayOfYear(rtc.getMonth(), rtc.getDay());
    uint16_t day_min = findMinutesOfDay(rtc.getHours(), rtc.getMinutes());
    uint8_t elapsed_time = findDayMinutesDifference(day_min, last_day_min);
    uint8_t emergency_data_count = backup_ready ? (elapsed_time / APD_PER_S) : 0;

    uint16_t addr = 1;
    saveInt(year_day, &addr);
    saveInt(day_min, &addr);
    eeprom.writeByte(addr++, emergency_data_count);
    if (emergency_data_count) {
        for (auto& vault : vaults) {
            vault->saveEmergencyData(emergency_data_count);
        }
    } else if (!backup_ready) {
        saveInt(out_temp.getHeadCount(), &addr);
        for (auto& vault : vaults) {
            vault->savePeriodicData(&addr);
        }
    }
    SAVE_BACKUP_STATE(true);
}

void restoreAuxiliaryData(uint16_t &addr, uint32_t &elapsed_time, uint16_t &periodic_cnt,
                          uint8_t &emergency_cnt, uint16_t &missing_cnt, uint16_t &start_index) {
    uint16_t curr_year_day = findDayOfYear(rtc.getMonth(), rtc.getDay());
    uint16_t curr_day_min = findMinutesOfDay(rtc.getHours(), rtc.getMinutes());
    elapsed_time = findYearMinutesDifference(curr_year_day, curr_day_min,
                                             readInt(&addr), readInt(&addr));

    emergency_cnt = eeprom.readByte(addr++);
    periodic_cnt = readInt(&addr);
    missing_cnt = elapsed_time / APD_PER_S;
    uint16_t total_count = periodic_cnt + emergency_cnt + missing_cnt;
    start_index = (total_count > DATA_PNTS_AMT) ? total_count - DATA_PNTS_AMT : 0;
}

void pullBackup() {
    uint16_t addr = 1;
    uint32_t elapsed_time;
    uint16_t periodic_cnt, missing_cnt, start_idx;
    uint8_t emergency_cnt;
    uint8_t curr_wday = rtc.getWeekDay(), curr_hour = rtc.getHours(), curr_min = rtc.getMinutes();

    restoreAuxiliaryData(addr, elapsed_time, periodic_cnt, emergency_cnt, missing_cnt, start_idx);
    if (elapsed_time < DATA_PNTS_AMT * APD_PER_S) {
        for (auto& vault : vaults) {
            vault->restorePointsData(&addr, start_idx, periodic_cnt, emergency_cnt, missing_cnt);
            vault->assignTimestamps(curr_wday, curr_hour, curr_min);
        }
    }
}