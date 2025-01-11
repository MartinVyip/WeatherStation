#ifndef MHZ19B_h
#define MHZ19B_h

#include <Arduino.h>

class MHZ19B {
public:
    MHZ19B(const uint8_t pwm_pin, const uint8_t hd_pin);

    uint16_t readCO2(bool update);
    void calibrateZero();

private:
    void measure();

    uint16_t _pulse_width;
    uint8_t _pwm_pin;
    uint8_t _hd_pin;
};

#endif