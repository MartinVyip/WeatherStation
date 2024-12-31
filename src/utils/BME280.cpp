#include <utils/BME280.h>

BME280::BME280(const uint8_t device_address, TwoWire *wire) 
    : _i2c_address(device_address), _wire(wire) {}

bool BME280::begin() {
    if (!reset()) return false;
    uint8_t ID = readRegister(0xD0);
    if (ID != 0x60 && ID != 0x58) return false;
    readCalibrationData();

    writeRegister(0xF2, _hum_oversampl);
    writeRegister(0xF2, readRegister(0xF2));
    writeRegister(0xF4, ((_temp_oversampl << 5) | (_press_oversampl << 2) | _operating_mode));
    writeRegister(0xF5, ((_standby_time << 5) | (_filter_coef << 2)));
    return true;
}

void BME280::setMode(uint8_t mode) {
    _operating_mode = mode;
}

void BME280::setFilter(uint8_t mode) {
    _filter_coef = mode;
}

void BME280::setStandbyTime(uint8_t mode) {
    _standby_time = mode;
}

void BME280::setHumOversampling(uint8_t mode) {
    _hum_oversampl = mode;
}

void BME280::setTempOversampling(uint8_t mode) {
    _temp_oversampl = mode;
}

void BME280::setPressOversampling(uint8_t mode) {
    _press_oversampl = mode;
}

int32_t BME280::readTempInt(void) {
    int32_t temp_raw = readRegister24(0xFA);
    if (temp_raw == 0x800000) return 0;

    temp_raw >>= 4;
    int32_t value_1 = ((((temp_raw >> 3) - ((int32_t)CalibrationData._T1 << 1))) * ((int32_t)CalibrationData._T2)) >> 11;
    int32_t value_2 = (((((temp_raw >> 4) - ((int32_t)CalibrationData._T1)) * ((temp_raw >> 4) - ((int32_t)CalibrationData._T1))) >> 12) * ((int32_t)CalibrationData._T3)) >> 14;
    return ((int32_t)value_1 + value_2);
}

float BME280::readTemperature(void) {
    int32_t temp_raw = readTempInt();
    float T = (temp_raw * 5 + 128) >> 8;
    return T / 100.0;
}

float BME280::readPressure(void) {
    uint32_t press_raw = readRegister24(0xF7);
    if (press_raw == 0x800000) return 0;

    press_raw >>= 4;
    int64_t value_1 = ((int64_t)readTempInt()) - 128000;
    int64_t value_2 = value_1 * value_1 * (int64_t)CalibrationData._P6;
    value_2 = value_2 + ((value_1 * (int64_t)CalibrationData._P5) << 17);
    value_2 = value_2 + (((int64_t)CalibrationData._P4) << 35);
    value_1 = ((value_1 * value_1 * (int64_t)CalibrationData._P3) >> 8) + ((value_1 * (int64_t)CalibrationData._P2) << 12);
    value_1 = (((((int64_t)1) << 47) + value_1)) * ((int64_t)CalibrationData._P1) >> 33;

    if (!value_1) return 0;

    int64_t p = 1048576 - press_raw;
    p = (((p << 31) - value_2) * 3125) / value_1;
    value_1 = (((int64_t)CalibrationData._P9) * (p >> 13) * (p >> 13)) >> 25;
    value_2 = (((int64_t)CalibrationData._P8) * p) >> 19;
    p = ((p + value_1 + value_2) >> 8) + (((int64_t)CalibrationData._P7) << 4);

    return (float)p / 256;
}

float BME280::readHumidity(void) {
    _wire->beginTransmission(_i2c_address);
    _wire->write(0xFD);
    if (_wire->endTransmission() != 0) return 0;
    _wire->requestFrom(_i2c_address, 2);
    int32_t hum_raw = ((uint16_t)_wire->read() << 8) | (uint16_t)_wire->read();
    if (hum_raw == 0x8000) return 0;

    int32_t value = (readTempInt() - ((int32_t)76800));
    value = (((((hum_raw << 14) - (((int32_t)CalibrationData._H4) << 20) - (((int32_t)CalibrationData._H5) * value)) + ((int32_t)16384)) >> 15) * (((((((value * ((int32_t)CalibrationData._H6)) >> 10) * (((value * ((int32_t)CalibrationData._H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)CalibrationData._H2) + 8192) >> 14));
    value = (value - (((((value >> 15) * (value >> 15)) >> 7) * ((int32_t)CalibrationData._H1)) >> 4));
    value = (value < 0) ? 0 : value;
    value = (value > 419430400) ? 419430400 : value;
    float h = (value >> 12);

    return h / 1024.0;
}

bool BME280::isMeasuring(void) {
    return (bool)((readRegister(0xF3) & 0x08) >> 3);
}

void BME280::oneMeasurement(void) {
    writeRegister(0xF4, ((readRegister(0xF4) & 0xFC) | 0x02));
}

bool BME280::reset(void) {
    if (!writeRegister(0xE0, 0xB6)) return false;
    delay(10);
    return true;
}

uint32_t BME280::readRegister24(uint8_t address) {
    _wire->beginTransmission(_i2c_address);
    _wire->write(address);
    if (_wire->endTransmission() != 0) return 0x800000;
    _wire->requestFrom(_i2c_address, 3);
    return (((uint32_t)_wire->read() << 16) | ((uint32_t)_wire->read() << 8) | (uint32_t)_wire->read());
}

bool BME280::writeRegister(uint8_t address, uint8_t data) {
    _wire->beginTransmission(_i2c_address);
    _wire->write(address);
    _wire->write(data);
    if (_wire->endTransmission() != 0) return false;
    return true;
}

uint8_t BME280::readRegister(uint8_t address) {
    _wire->beginTransmission(_i2c_address);
    _wire->write(address);
    if (_wire->endTransmission() != 0) return 0;
    _wire->requestFrom(_i2c_address, 1);
    return _wire->read();
}

void BME280::readCalibrationData(void) {
    _wire->beginTransmission(_i2c_address);
    _wire->write(0x88);
    if (_wire->endTransmission() != 0) return;
    _wire->requestFrom(_i2c_address, 26);

    CalibrationData._T1 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._T2 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._T3 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P1 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P2 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P3 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P4 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P5 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P6 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P7 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P8 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._P9 = (_wire->read() | (_wire->read() << 8));
    _wire->read();
    CalibrationData._H1 = _wire->read();

    _wire->beginTransmission(_i2c_address);
    _wire->write(0xE1);
    _wire->endTransmission();
    _wire->requestFrom(_i2c_address, 8);

    CalibrationData._H2 = (_wire->read() | (_wire->read() << 8));
    CalibrationData._H3 = _wire->read();
    CalibrationData._H4 = (_wire->read() << 4);
    uint8_t interVal = _wire->read();
    CalibrationData._H4 |= (interVal & 0xF);
    CalibrationData._H5 = (((interVal & 0xF0) >> 4) | (_wire->read() << 4));
    CalibrationData._H6 = _wire->read();
}