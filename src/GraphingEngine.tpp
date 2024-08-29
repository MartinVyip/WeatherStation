template <typename input_type>
Graph<input_type>::Graph(DataVault<input_type>& data_reference, Adafruit_ILI9341& tft_reference)
    : _data(data_reference), _tft(tft_reference) {
}

template <typename input_type>
void Graph<input_type>::drawStatic(bool local_sizing, int16_t endpoint) {
    endpoint = (endpoint < _data.getHeadCount()) ? endpoint : _data.getHeadCount() - 1;
    staticGraphCore(endpoint, local_sizing);
}

template <typename input_type>
void Graph<input_type>::drawFreshStatic(bool local_sizing) {
    int16_t endpoint = _data.getHeadCount() - 1;
    staticGraphCore(endpoint, local_sizing);
}

template <typename input_type>
void Graph<input_type>::dynamicChange(int8_t step) {

}

template <typename input_type>
void Graph<input_type>::staticGraphCore(int16_t endp, bool local_sizing) {
    int16_t startp, graph_startp, graph_endp;

    _tft.fillScreen(0x0000);
    startp = graph_startp = (endp + L_EDGE - TFT_XMAX < 0) ? 0 : endp + L_EDGE - TFT_XMAX;
    graph_endp = endp;
    if (!local_sizing) {
        endp = _data.getHeadCount() - 1;
        startp = 0;
    }
    input_type sample_max = _data.findSampleMax(startp, endp);
    input_type sample_min = _data.findSampleMin(startp, endp);
    uint8_t axis_level = findAxisLevel(sample_min, sample_max);
    
    drawCurve(graph_startp, graph_endp, sample_min, sample_max, axis_level);
    drawAxises(axis_level);
    drawTicks(graph_startp, graph_endp);
}

template <typename input_type>
void Graph<input_type>::drawCurve(int16_t startpoint, int16_t endpoint,
                                  input_type min_value, input_type max_value, uint8_t axis_level) {
    for (uint16_t i = startpoint; i <= endpoint; i++) {
        uint16_t x = i - startpoint + L_EDGE;
        uint8_t h = round(mapFloat(_data.getData()[i].value, min_value, max_value, BT_EDGE - UP_EDGE, 0));
        if (min_value >= 0) {
            _tft.drawFastVLine(x, BT_EDGE, -h, PLOT_COLOR);
        }
        else if (max_value <= 0) {
            _tft.drawFastVLine(x, UP_EDGE, h, PLOT_COLOR);
        }
        else _tft.drawFastVLine(x, axis_level, h - axis_level + UP_EDGE, PLOT_COLOR);
    }
}

template <typename input_type>
void Graph<input_type>::drawAxises(uint8_t axis_level) {
    _tft.drawFastVLine(L_EDGE, UP_EDGE - TICK_LEN, BT_EDGE - UP_EDGE + TICK_LEN, AXIS_COLOR);
    _tft.drawFastHLine(L_EDGE - TICK_LEN, axis_level, TFT_XMAX - L_EDGE + TICK_LEN, AXIS_COLOR);
    if (axis_level != BT_EDGE) {
        _tft.drawFastHLine(L_EDGE - TICK_LEN, BT_EDGE, TFT_XMAX - L_EDGE + TICK_LEN, AXIS_COLOR);
    }
}

template <typename input_type>
void Graph<input_type>::drawTicks(uint16_t startpoint, uint16_t endpoint) {
    _tft.setTextColor(TEXT_COLOR);
    for (uint8_t i = 0; i * TICK_PER < 24; i++) {
        if (_tick_indexes[i] != -1) {
            _tft.drawFastVLine(_tick_indexes[i], BT_EDGE + 1, TICK_LEN, 0x0000);
            _tft.fillRect(_tick_indexes[i] - 14, BT_EDGE + 7, 31, 15, 0x0000);
            _tick_indexes[i] = -1;
        }
    }
    for (uint8_t i = 0; i * TICK_PER < 24; i++) {
        uint8_t tick = i * TICK_PER;
        int8_t min_diff = 31;
        for (uint16_t j = startpoint; j <= endpoint; j++) {
            if (_data.getData()[j].hour == tick) {
                int8_t diff = min((int8_t)_data.getData()[j].minute, int8_t(60 - _data.getData()[j].minute));
                if (diff < min_diff) {
                    min_diff = diff;
                    _tick_indexes[i] = j - startpoint + L_EDGE;
                }
            }
        }
        if (_tick_indexes[i] != -1 && _tick_indexes[i] < TFT_XMAX - 15) {
            char hours[2];
            _tft.drawFastVLine(_tick_indexes[i], BT_EDGE, TICK_LEN, TICK_COLOR);
            itoa(tick, hours, DEC);
            _tft.setCursor(_tick_indexes[i] - 6 * strlen(hours) - 2, BT_EDGE + 7);
            _tft.print(hours);
            _tft.setCursor(_tick_indexes[i] + 4, BT_EDGE + 7);
            _tft.print("00");
        }
    }
}

template <typename input_type>
uint8_t Graph<input_type>::findAxisLevel(input_type min_value, input_type max_value) {
    uint8_t axis_level;
    if (min_value >= 0) axis_level = BT_EDGE;
    else if (max_value <= 0) axis_level = UP_EDGE;
    else axis_level = round(mapFloat(0, min_value, max_value, BT_EDGE, UP_EDGE));
    return axis_level;
}

template <typename input_type>
float Graph<input_type>::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}