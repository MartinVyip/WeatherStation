#include <utils/MHZ19B.h>

MHZ19B::MHZ19B(const uint8_t pwm_pin, const uint8_t hd_pin)
    : _pwm_pin(pwm_pin), _hd_pin(hd_pin) {}

uint16_t MHZ19B::readCO2(bool update) {
    if (_pulse_width == 0 || update) measure();
    return ((_pulse_width - 2) << 1);
}

void MHZ19B::calibrateZero() {
    pinMode(_hd_pin, OUTPUT);
    digitalWrite(_hd_pin, LOW);
    delay(10000);
    digitalWrite(_hd_pin, HIGH);
}

void MHZ19B::measure() {
    if (digitalRead(_pwm_pin)) {
        while (digitalRead(_pwm_pin));
    }
    while (!digitalRead(_pwm_pin));

    uint32_t start = millis();
    while (digitalRead(_pwm_pin));
    _pulse_width = millis() - start;
}