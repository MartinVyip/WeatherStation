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
void DataVault<input_type>::appendValue(input_type value, uint8_t hour, uint8_t minute) {
    if (_head_count < _buffer_size) {
        _data[_head_count] = {value, 1, hour, minute};
        _head_count++;
    }
    else {
        for (uint16_t i = 1; i < _buffer_size; i++) {
            _data[i - 1] = _data[i];
        }
        _data[_buffer_size - 1] = {value, 1, hour, minute};
    }
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