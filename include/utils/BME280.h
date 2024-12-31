#ifndef BME280_h
#define BME280_h

#include <Arduino.h>
#include <Wire.h>

#define NORMAL_MODE 0x03
#define FORCED_MODE 0x02

#define STANDBY_500US 0x00
#define STANDBY_10MS 0x06
#define STANDBY_20MS 0x07
#define STANDBY_6250US 0x01
#define STANDBY_125MS 0x02
#define STANDBY_250MS 0x03
#define STANDBY_500MS 0x04
#define STANDBY_1000MS 0x05

#define MODULE_DISABLE 0x00
#define OVERSAMPLING_1 0x01
#define OVERSAMPLING_2 0x02
#define OVERSAMPLING_4 0x03
#define OVERSAMPLING_8 0x04
#define OVERSAMPLING_16 0x05

#define FILTER_DISABLE 0x00
#define FILTER_COEF_2 0x01
#define FILTER_COEF_4 0x02
#define FILTER_COEF_8 0x03
#define FILTER_COEF_16 0x04

class BME280 {
public:
    BME280(const uint8_t device_address, TwoWire *wire);

    bool begin();
    bool isMeasuring();
    float readPressure();
    float readHumidity();
    float readTemperature();
    void oneMeasurement();

    void setMode(uint8_t mode);
    void setFilter(uint8_t mode);
    void setStandbyTime(uint8_t mode);
    void setHumOversampling(uint8_t mode);
    void setTempOversampling(uint8_t mode);
    void setPressOversampling(uint8_t mode);

private:
    bool reset();
    int32_t readTempInt();
    void readCalibrationData(void);
    uint8_t readRegister(uint8_t address);
    uint32_t readRegister24(uint8_t address);
    bool writeRegister(uint8_t address, uint8_t data);

    int16_t _i2c_address = 0x76;
    TwoWire * _wire = &Wire;
    uint8_t _operating_mode = NORMAL_MODE;
    uint8_t _standby_time = STANDBY_250MS;
    uint8_t _filter_coef = FILTER_COEF_16;
    uint8_t _temp_oversampl = OVERSAMPLING_4;
    uint8_t _hum_oversampl = OVERSAMPLING_1;
    uint8_t _press_oversampl = OVERSAMPLING_2;

    struct {
        uint16_t _T1;
        int16_t _T2;
        int16_t _T3;
        uint16_t _P1;
        int16_t _P2;
        int16_t _P3;
        int16_t _P4;
        int16_t _P5;
        int16_t _P6;
        int16_t _P7;
        int16_t _P8;
        int16_t _P9;
        uint8_t _H1;
        int16_t _H2;
        uint8_t _H3;
        int16_t _H4;
        int16_t _H5;
        int8_t _H6;
    } CalibrationData;
};

#endif