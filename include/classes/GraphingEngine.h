#ifndef GraphingEngine_h
#define GraphingEngine_h

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

#include <classes/DataVault.h>
#include <config/Constants.h>

class GraphBase {
public:
    virtual void drawLocal(bool local_sizing = true) = 0;
    virtual void drawFresh(bool local_sizing = true) = 0;
    virtual void drawCursor(bool initial = false) = 0;
    virtual void dynamicPan(int8_t step) = 0;
    virtual void dynamicCursor(int8_t step) = 0;
    virtual void annotate(bool dayscale = true) = 0;
    virtual void drawLogos(enum screens screen, bool high) = 0;

    virtual ~GraphBase() {}
};

template <typename input_type>
class Graph : public GraphBase {
public:
    Graph(DataVault<input_type>& data_ref, Adafruit_ILI9341& tft_ref);
    ~Graph() override = default;

    void drawLocal(bool local_sizing = true) override;
    void drawFresh(bool local_sizing = true) override;
    void drawCursor(bool initial = false) override;
    void dynamicPan(int8_t step) override;
    void dynamicCursor(int8_t step) override;
    void annotate(bool dayscale = true) override;
    void drawLogos(enum screens screen, bool high) override;

    static uint16_t getTextWidth(const char* string, Adafruit_ILI9341& tft_reference);

private:
    DataVault<input_type>& _data;
    Adafruit_ILI9341& _tft;

    // Curve management
    int16_t _curr_startp, _curr_endp;
    uint8_t _curr_level;
    uint8_t _prev_values[TFT_XMAX - L_EDGE];
    input_type _curr_max, _curr_min;

    void staticGraphCore(int16_t endp, bool local_sizing);
    void updateCurve(bool initial = false);
    void updateAxises(bool initial = false);

    // Ticks management
    int16_t _tick_posns[24 / TICK_PER];
    void updateTicks(bool initial = false);

    // Weekday management
    uint16_t _separtr_index;
    uint16_t _spot_posns[2];
    uint8_t _spot_lengths[2];
    void updateWeekdays(bool initial = false);

    // Cursor management
    int16_t _curr_index, _prev_index;
    uint16_t _cursor_x;
    uint8_t _window_width;

    void drawCursorPointer();
    void drawCursorData();
    void erasePrevCursor();
    void eraseCursorRect();
    void eraseCursorLine();
    void eraseCursorData();

    // Auxiliary
    void findAxisLevel();
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    uint16_t findDataEdge();
};

#include <classes/GraphingEngine.tpp>

#endif