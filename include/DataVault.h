#ifndef DataVault_h
#define DataVault_h

#include <Arduino.h>

#include <Constants.h>

template <typename input_type>
struct DataPoint {
    input_type value;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
};

template <typename input_type>
class DataVault {
public:
    DataVault(uint16_t data_size);
    ~DataVault();

    void appendToVault(uint8_t wday, uint8_t hour, uint8_t min);
    void appendToAverage(input_type value);
    input_type findSampleMax(uint16_t startpoint, uint16_t endpoint) const;
    input_type findSampleMin(uint16_t startpoint, uint16_t endpoint) const;

    const DataPoint<input_type>* getData() const;
    uint16_t getHeadCount() const;
    void getCharTime(uint16_t index, char* buffer) const;
    static void getCharValue(input_type value, char* buffer);

private:
    DataPoint<input_type>* _data;

    uint16_t _buffer_size, _head_count;
    input_type _average_sum;
    uint8_t _average_counter;
};

#include <DataVault.tpp>

#endif