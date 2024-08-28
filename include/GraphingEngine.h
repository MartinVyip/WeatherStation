#ifndef GraphingEngine_h
#define GraphingEngine_h

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

#include <Constants.h>
#include <GraphingEngine.tpp>

template <typename input_type>
class Graph {
public:
    Graph(uint16_t data_size, Adafruit_ILI9341& tft);
    ~Graph();
    void appendValue(input_type value, uint8_t a, uint8_t b);
    void drawStaticGraph(bool local_sizing = true, int16_t endpoint = INT16_MAX);
    void drawFreshStaticGraph(bool local_sizing = true);
    void graphDynamicChange(int8_t step);

private:
    Adafruit_ILI9341& tft;
    struct DataPoint {
        input_type value;
        uint8_t weekday;
        uint8_t hour;
        uint8_t minute;
    };

    DataPoint* _data;
    uint16_t _buffer_size;
    uint16_t _head_count;
    int16_t _tick_indexes[24 / TICK_PER];

    void staticGraphCore(int16_t endp, bool local_sizing);
    void drawCurve(int16_t startpoint, int16_t endpoint, input_type min_value, input_type max_value, uint8_t axis_level);
    void drawAxises(uint8_t axis_level);
    void drawTicks(uint16_t startpoint, uint16_t endpoint);
    uint8_t findAxisLevel(input_type min_value, input_type max_value);
    input_type findSampleMax(uint16_t startpoint, uint16_t endpoint);
    input_type findSampleMin(uint16_t startpoint, uint16_t endpoint);
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif