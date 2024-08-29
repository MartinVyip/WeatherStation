#ifndef GraphingEngine_h
#define GraphingEngine_h

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

#include <Constants.h>
#include <DataVault.h>

template <typename input_type>
class Graph {
public:
    Graph(DataVault<input_type>& data_reference, Adafruit_ILI9341& tft_reference);

    void drawStatic(bool local_sizing = true, int16_t endpoint = INT16_MAX);
    void drawFreshStatic(bool local_sizing = true);
    void dynamicChange(int8_t step);

private:
    DataVault<input_type>& _data;
    Adafruit_ILI9341& _tft;
    int16_t _tick_indexes[24 / TICK_PER];

    void staticGraphCore(int16_t endp, bool local_sizing);
    void drawCurve(int16_t startpoint, int16_t endpoint, input_type min_value, input_type max_value, uint8_t axis_level);
    void drawAxises(uint8_t axis_level);
    void drawTicks(uint16_t startpoint, uint16_t endpoint);
    uint8_t findAxisLevel(input_type min_value, input_type max_value);
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
};

#include <GraphingEngine.tpp>

#endif