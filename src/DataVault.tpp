template <typename input_type>
DataVault<input_type>::DataVault(uint16_t data_size) {
    _data = new DataPoint<input_type>[data_size];
    _buffer_size = data_size;
}

template <typename input_type>
DataVault<input_type>::~DataVault() {
    delete[] _data;
}

template <typename input_type>
void DataVault<input_type>::appendValue(input_type value, uint8_t wday, uint8_t hour, uint8_t min) {
    if (_head_count < _buffer_size) {
        _data[_head_count] = {value, wday, hour, min};
        _head_count++;
    } else {
        for (uint16_t i = 1; i < _buffer_size; i++) {
            _data[i - 1] = _data[i];
        }
        _data[_buffer_size - 1] = {value, wday, hour, min};
    }
    _average_counter = _average_sum = 0;
}

template <typename input_type>
input_type DataVault<input_type>::appendToAverage(input_type value) {
    _average_sum += value;
    _average_counter++;
    return (_average_sum / _average_counter);
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
const DataPoint<input_type>* DataVault<input_type>::getData() const {
    return _data;
}

template <typename input_type>
uint16_t DataVault<input_type>::getHeadCount() const {
    return _head_count;
}

template <typename input_type>
void DataVault<input_type>::getCharValue(input_type value, char* buffer) const {
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