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
    void annotate(bool dayscale = true);

private:
    DataVault<input_type>& _data;
    Adafruit_ILI9341& _tft;

    const char _weekdays[7][12] = {
        {'E', 's', 'm', 'a', 's', 'p', 0x85, 'e', 'v', '\0'},
        {'T', 'e', 'i', 's', 'i', 'p', 0x84, 'e', 'v', '\0'},
        {'K', 'o', 'l', 'm', 'a', 'p', 0x84, 'e', 'v', '\0'},
        {'N', 'e', 'l', 'j', 'a', 'p', 0x84, 'e', 'v', '\0'},
        {'R', 'e', 'e', 'd', 'e', '\0'},
        {'L', 'a', 'u', 'p', 0x84, 'e', 'v', '\0'},
        {'P', 0x81, 'h', 'a', 'p', 0x84, 'e', 'v', '\0'}
    };
    uint16_t _separtr_index;
    uint16_t _spot_posns[2];
    uint8_t _spot_lengths[2];

    int16_t _tick_posns[24 / TICK_PER];

    uint16_t _curr_startp, _curr_endp;
    uint8_t _curr_level;
    uint8_t _prev_values[TFT_XMAX - L_EDGE];
    input_type _curr_max, _curr_min;

    int16_t _curr_index, _prev_index;

    void staticGraphCore(int16_t endp, bool local_sizing);
    void updateCurve(bool initial = false);
    void updateAxises(bool initial = false);
    void updateTicks(bool initial = false);
    void updateWeekday(bool initial = false);
    void updateCursor(bool initial = false);

    void findAxisLevel();
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
};

#include <GraphingEngine.tpp>

#endif