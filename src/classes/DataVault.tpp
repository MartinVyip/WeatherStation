template <typename input_type>
DataVault<input_type>::DataVault(I2C_eeprom& eeprom_ref) 
    : _eeprom(eeprom_ref) {}

template <typename input_type>
DataVault<input_type>::DataVault(float slope_norm, I2C_eeprom& eeprom_ref) 
    : _norm_coef(slope_norm), _eeprom(eeprom_ref) {}

template <typename input_type>
void DataVault<input_type>::appendToVault(uint8_t wday, uint8_t hour, uint8_t min) {
    input_type value;
    if constexpr (std::is_integral<input_type>::value) {
        value = round((float) _average_sum / _average_counter);
    } else if constexpr (std::is_floating_point<input_type>::value) {
        value = (float) _average_sum / _average_counter;
    }
    _average_sum = value;
    _average_counter = 1;

    if (_head_count < DATA_PNTS_AMT) {
        _data[_head_count] = {value, wday, hour, min};
        _head_count++;
    } else {
        for (uint16_t i = 1; i < DATA_PNTS_AMT; i++) {
            _data[i - 1] = _data[i];
        }
        _data[DATA_PNTS_AMT - 1] = {value, wday, hour, min};
    }
}

template <typename input_type>
void DataVault<input_type>::appendToAverage(input_type value) {
    _average_sum += value;
    _average_counter++;
}

template <typename input_type>
input_type DataVault<input_type>::findSampleMax(uint16_t startpoint, uint16_t endpoint) const {
    input_type max_value = _data[startpoint].value;
    for (uint16_t i = startpoint + 1; i <= endpoint; i++) {
        max_value = max(max_value, _data[i].value);
    }
    return max_value;
}

template <typename input_type>
input_type DataVault<input_type>::findSampleMin(uint16_t startpoint, uint16_t endpoint) const {
    input_type min_value = _data[startpoint].value;
    for (uint16_t i = startpoint + 1; i <= endpoint; i++) {
        min_value = min(min_value, _data[i].value);
    }
    return min_value;
}

template <typename input_type>
int8_t DataVault<input_type>::findNormalizedTrendSlope(uint8_t period) const {
    if (_head_count == 0) return 0;

    uint16_t start_index = findStartIndex(period);
    uint16_t start_time = findMinutesOfDay(_data[start_index].hour, _data[start_index].minute);

    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    uint8_t n = _head_count - start_index;
    for (uint16_t i = start_index; i < _head_count; ++i) {
        uint16_t point_time = findMinutesOfDay(_data[i].hour, _data[i].minute);

        float y = _data[i].value;
        uint16_t x = findDayMinutesDifference(point_time, start_time);
        sum_x += x; sum_y += y;
        sum_xy += x * y; sum_x2 += x * x;
    }

    float numerator = n * sum_xy - sum_x * sum_y;
    float denominator = n * sum_x2 - sum_x * sum_x;
    float slope = (denominator == 0) ? 0 : numerator / denominator;

    return (_norm_coef != 0) ? normalizeSlope(slope) : slope;
}

template <typename input_type>
void DataVault<input_type>::savePeriodicData(uint16_t* curr_addr) {
    uint16_t data_len = _head_count << 1;
    uint8_t data_arr[data_len];

    for (uint16_t i = 0; i < _head_count; i++) {
        uint8_t bytes[2];
        getBytesFromValue(_data[i].value, bytes);
        STORE_BYTES(data_arr, i, bytes);
    }
    _eeprom.writeBlock(*curr_addr, data_arr, data_len);
    *curr_addr += data_len;

    _emergency_addr = *curr_addr;
    *curr_addr += BYTES_PER_HOUR;
}

template <typename input_type>
void DataVault<input_type>::saveEmergencyData(uint8_t new_data_cnt) {
    uint16_t data_len = new_data_cnt << 1;
    uint8_t data_arr[data_len];

    for (uint8_t i = 0; i < new_data_cnt; i++) {
        uint8_t bytes[2];
        getBytesFromValue(_data[_head_count - new_data_cnt + i].value, bytes);
        STORE_BYTES(data_arr, i, bytes);
    }
    _eeprom.writeBlock(_emergency_addr, data_arr, data_len);
}

template <typename input_type>
void DataVault<input_type>::restorePointsData(uint16_t* curr_addr, 
                                              uint16_t st_index, uint16_t per_count,
                                              uint8_t em_count, uint16_t miss_count) {
    _head_count = 0;
    if (st_index < per_count) {
        uint16_t data_len = per_count << 1;
        uint8_t periodic_data[data_len];
        _eeprom.readBlock(*curr_addr, periodic_data, data_len);

        for (uint16_t i = st_index; i < per_count; i++) {
            _data[_head_count++].value = getValueFromBytes(&periodic_data[i << 1]);
        }
        *curr_addr += data_len;
    }

    if (em_count && st_index < per_count + em_count) {
        uint16_t data_len = em_count << 1;
        uint8_t emergency_data[data_len];
        _eeprom.readBlock(*curr_addr, emergency_data, data_len);
        st_index = max(st_index - per_count, 0);

        for (uint16_t i = st_index; i < em_count; i++) {
            _data[_head_count++].value = getValueFromBytes(&emergency_data[i << 1]);
        }
    }

    if (miss_count && _head_count) {
        input_type last_point = _data[_head_count - 1].value;
        for (uint16_t i = 0; i < miss_count && _head_count < DATA_PNTS_AMT; i++) {
            _data[_head_count++].value = last_point;
        }
    }
    *curr_addr += BYTES_PER_HOUR;
}

template <typename input_type>
void DataVault<input_type>::assignTimestamps(uint8_t curr_wday, uint8_t curr_hour, uint8_t curr_min) {
    for (int16_t i = _head_count - 1; i >= 0; i--) {
        _data[i].weekday = curr_wday - 1;
        _data[i].hour = curr_hour;
        _data[i].minute = curr_min;

        if (curr_min >= APD_PER_S) {
            curr_min -= APD_PER_S;
        } else {
            uint8_t decrement = APD_PER_S - curr_min;
            curr_min = 60 - decrement;
            if (curr_hour > 0) {
                curr_hour--;
            } else {
                curr_hour = 23;
                curr_wday = (curr_wday > 1) ? curr_wday - 1 : 7;
            }
        }
    }
}

template <typename input_type>
const DataPoint<input_type>* DataVault<input_type>::getData() const {
    return _data;
}

template <typename input_type>
input_type DataVault<input_type>::getLastValue() const {
    if (_head_count == 0) return 0;
    return _data[_head_count - 1].value;
}

template <typename input_type>
uint16_t DataVault<input_type>::getHeadCount() const {
    return _head_count;
}

template <typename input_type>
void DataVault<input_type>::getCharTime(uint16_t index, char* buffer) const {
    sprintf(buffer, "%u:%02u", _data[index].hour, _data[index].minute);
}

template <typename input_type>
void DataVault<input_type>::getCharValue(input_type value, char* buffer) {
    if constexpr (std::is_floating_point<input_type>::value) {
        int16_t int_part = static_cast<int>(value);
        float frac_part = value - int_part;
        int16_t round_frac = static_cast<int>(round(frac_part * 10));
        if (abs(round_frac) >= 10) {
            int_part = (round_frac > 0) ? ++int_part : --int_part;
            round_frac = 0;
        }
        sprintf(buffer, "%d.%1d", int_part, abs(round_frac));
    } else if constexpr (std::is_integral<input_type>::value) {
        sprintf(buffer, "%d", value);
    }
}

template <typename input_type>
void DataVault<input_type>::getBytesFromValue(input_type value, uint8_t* bytes) const {
    int16_t curr_value;
    if constexpr (std::is_floating_point<input_type>::value) {
        curr_value = static_cast<int>(round(value * 10));
    } else if constexpr (std::is_integral<input_type>::value) {
        curr_value = static_cast<int>(value);
    }
    bytes[0] = (curr_value >> 8) & 0xFF;
    bytes[1] = curr_value & 0xFF;
}

template <typename input_type>
input_type DataVault<input_type>::getValueFromBytes(uint8_t* bytes) const {
    int16_t curr_value = (bytes[0] << 8) | bytes[1];
    if constexpr (std::is_floating_point<input_type>::value) {
        return (float) static_cast<float>(curr_value) / 10;
    } else if constexpr (std::is_integral<input_type>::value) {
        return static_cast<input_type>(curr_value);
    }
}

template <typename input_type>
uint16_t DataVault<input_type>::findStartIndex(uint8_t backstep_time_period) const {
    uint16_t end_time = findMinutesOfDay(_data[_head_count - 1].hour, _data[_head_count - 1].minute);

    for (int16_t i = _head_count - 1; i >= 0; --i) {
        uint16_t point_time = findMinutesOfDay(_data[i].hour, _data[i].minute);
        if (findDayMinutesDifference(end_time, point_time) >= backstep_time_period) {
            return i;
        }
    }
    return 0;
}

template <typename input_type>
int8_t DataVault<input_type>::normalizeSlope(float slope) const {
    return constrain((float) 100 * (slope / _norm_coef), -100, 100);
}