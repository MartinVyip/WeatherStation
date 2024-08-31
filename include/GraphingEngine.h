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

    void drawLocal(bool local_sizing = true);
    void drawFresh(bool local_sizing = true);
    void drawCursor();
    void dynamicPan(int8_t step);
    void dynamicCursor(int8_t step);
    void annotate();

private:
    DataVault<input_type>& _data;
    Adafruit_ILI9341& _tft;

    int16_t _tick_indexes[24 / TICK_PER];
    int16_t _curr_startp, _curr_endp;
    uint8_t _curr_level;
    uint8_t _prev_values[TFT_XMAX - L_EDGE];
    input_type _curr_max, _curr_min;

    void staticGraphCore(int16_t endp, bool local_sizing);
    void updateCurve(bool initial = false);
    void updateAxises(bool initial = false);
    void updateTicks(bool initial = false);

    void findAxisLevel();
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
};

#include <GraphingEngine.tpp>

#endif