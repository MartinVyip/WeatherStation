template <typename input_type>
DataVault<input_type>::DataVault(uint16_t data_size) {
    _data = new DataPoint<input_type>[data_size];
    _buffer_size = data_size;
}

template <typename input_type>
DataVault<input_type>::DataVault(uint16_t data_size, float slope_normalization) {
    _data = new DataPoint<input_type>[data_size];
    _buffer_size = data_size;
    _norm_coef = slope_normalization;
}

template <typename input_type>
DataVault<input_type>::~DataVault() {
    delete[] _data;
}

template <typename input_type>
void DataVault<input_type>::appendToVault(uint8_t wday, uint8_t hour, uint8_t min) {
    input_type value = _average_sum / _average_counter;
    _average_counter = _average_sum = 0;

    if (_head_count < _buffer_size) {
        _data[_head_count] = {value, wday, hour, min};
        _head_count++;
    } else {
        for (uint16_t i = 1; i < _buffer_size; i++) {
            _data[i - 1] = _data[i];
        }
        _data[_buffer_size - 1] = {value, wday, hour, min};
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
    uint16_t start_time = convertToMinutes(
        _data[start_index].weekday,
        _data[start_index].hour,
        _data[start_index].minute
    );

    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    uint8_t n = _head_count - start_index;
    for (uint16_t i = start_index; i < _head_count; ++i) {
        uint16_t point_time = convertToMinutes(
            _data[i].weekday, _data[i].hour, _data[i].minute
        );

        float y = _data[i].value;
        uint16_t x = (point_time >= start_time)
            ? point_time - start_time
            : (7 * 24 * 60) - start_time + point_time;

        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    float numerator = n * sum_xy - sum_x * sum_y;
    float denominator = n * sum_x2 - sum_x * sum_x;
    float slope = (denominator == 0) ? 0 : numerator / denominator;

    return (_norm_coef != 0) ? normalizeSlope(slope) : slope;
}

template <typename input_type>
const DataPoint<input_type>* DataVault<input_type>::getData() const {
    return _data;
}

template <typename input_type>
input_type DataVault<input_type>::getLastValue() const {
    return _data[_head_count - 1].value;
}

template <typename input_type>
uint16_t DataVault<input_type>::getHeadCount() const {
    return _head_count;
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
void DataVault<input_type>::getCharTime(uint16_t index, char* buffer) const {
    sprintf(buffer, "%u:%02u", _data[index].hour, _data[index].minute);
}

template <typename input_type>
uint16_t DataVault<input_type>::convertToMinutes(uint8_t wday, uint8_t hour, uint8_t min) const {
    return (wday - 1) * 24 * 60 + hour * 60 + min;
}

template <typename input_type>
uint16_t DataVault<input_type>::findStartIndex(uint8_t backstep_time_period) const {
    uint16_t end_time = convertToMinutes(
        _data[_head_count - 1].weekday,
        _data[_head_count - 1].hour,
        _data[_head_count - 1].minute
    );

    for (int16_t i = _head_count - 1; i >= 0; --i) {
        uint16_t point_time = convertToMinutes(
            _data[i].weekday, _data[i].hour, _data[i].minute
        );

        uint16_t time_diff = (end_time >= point_time)
            ? end_time - point_time
            : (7 * 24 * 60) - point_time + end_time;

        if (time_diff >= backstep_time_period) {
            return i;
        }
    }
    return 0;
}

template <typename input_type>
int8_t DataVault<input_type>::normalizeSlope(float slope) const {
    return constrain((float) 100 * (slope / _norm_coef), -100, 100);
}