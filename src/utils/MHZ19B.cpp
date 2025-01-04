#include <utils/MHZ19B.h>

MHZ19B::MHZ19B(const uint8_t pwm_pin, const uint8_t hd_pin)
    : _pwm_pin(pwm_pin), _hd_pin(hd_pin) {
    pinMode(_pwm_pin, INPUT);
    pinMode(_hd_pin, OUTPUT);
    digitalWrite(_hd_pin, HIGH);
}

uint16_t MHZ19B::readCO2(bool update) {
    if (_pulse_width == 0 || update) measure();
    return ((_pulse_width - 2) << 2);
}

void MHZ19B::calibrateZero() {
    digitalWrite(_hd_pin, LOW);
    delay(10000);
    digitalWrite(_hd_pin, HIGH);
}

void MHZ19B::measure() {
    uint32_t start_time = millis();
    if (digitalRead(_pwm_pin)) {
        while (digitalRead(_pwm_pin)) {
            if (millis() - start_time > 2000) return;
        };
    }
    start_time = millis();
    while (!digitalRead(_pwm_pin)) {
        if (millis() - start_time > 2000) return;
    };

    start_time = millis();
    while (digitalRead(_pwm_pin)) {
        if (millis() - start_time > 2000) return;
    };
    _pulse_width = millis() - start_time;
}