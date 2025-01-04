#ifndef DataVault_h
#define DataVault_h

#include <Arduino.h>
#include <I2C_eeprom.h>

#include <utils/TimeUtils.h>
#include <config/Constants.h>

template <typename input_type>
struct DataPoint {
    input_type value;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
};

class VaultBase {
public:
    virtual void appendToVault(uint8_t wday, uint8_t hour, uint8_t min) = 0;
    virtual void savePeriodicData(uint16_t* curr_addr) = 0;
    virtual void saveEmergencyData(uint8_t emergency_data_count) = 0;
    virtual void restorePointsData(uint16_t* curr_addr, uint16_t st_index,
                                   uint16_t per_count, uint8_t em_count, uint16_t miss_count) = 0;
    virtual void assignTimestamps(uint8_t curr_wday, uint8_t curr_hour, uint8_t curr_min) = 0;

    virtual ~VaultBase() {}
};

template <typename input_type>
class DataVault : public VaultBase {
public:
    DataVault(I2C_eeprom& _eeprom_ptr);
    DataVault(float slope_norm, I2C_eeprom& eeprom_ptr);

    void appendToVault(uint8_t wday, uint8_t hour, uint8_t min) override;
    void appendToAverage(input_type value);
    input_type findSampleMax(uint16_t startpoint, uint16_t endpoint) const;
    input_type findSampleMin(uint16_t startpoint, uint16_t endpoint) const;
    int8_t findNormalizedTrendSlope(uint8_t period) const;

    void savePeriodicData(uint16_t* curr_addr) override;
    void saveEmergencyData(uint8_t new_data_points) override;
    void restorePointsData(uint16_t* curr_addr, uint16_t st_index,
                           uint16_t per_count, uint8_t em_count, uint16_t miss_count) override;
    void assignTimestamps(uint8_t curr_wday, uint8_t curr_hour, uint8_t curr_min) override;

    const DataPoint<input_type>* getData() const;
    input_type getLastValue() const;
    uint16_t getHeadCount() const;
    void getCharTime(uint16_t index, char* buffer) const;
    static void getCharValue(input_type value, char* buffer, bool forced_round = false);

private:
    DataPoint<input_type> _data[DATA_PNTS_AMT];
    I2C_eeprom& _eeprom;

    uint16_t _head_count, _emergency_addr;
    input_type _average_sum;
    uint8_t _average_counter;
    float _norm_coef = 0;

    void getBytesFromValue(input_type value, uint8_t* bytes) const;
    input_type getValueFromBytes(uint8_t* bytes) const;
    uint16_t findStartIndex(uint8_t backstep_time_period) const;
    int8_t normalizeSlope(float slope) const;
};

#include <classes/DataVault.tpp>

#endif