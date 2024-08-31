template <typename input_type>
Graph<input_type>::Graph(DataVault<input_type>& data_reference, Adafruit_ILI9341& tft_reference)
    : _data(data_reference), _tft(tft_reference) {
}

template <typename input_type>
void Graph<input_type>::drawLocal(bool local_sizing) {
    staticGraphCore(_curr_endp, local_sizing);
}

template <typename input_type>
void Graph<input_type>::drawFresh(bool local_sizing) {
    staticGraphCore(_data.getHeadCount() - 1, local_sizing);
}

template <typename input_type>
void Graph<input_type>::drawCursor() {

}

template <typename input_type>
void Graph<input_type>::dynamicPan(int8_t step) {
    if (_data.getHeadCount() > TFT_XMAX - L_EDGE) {
        int16_t prev_startp = _curr_startp;

        _curr_startp += step;
        _curr_endp += step;
        _curr_startp = constrain(_curr_startp, 0, _data.getHeadCount() + L_EDGE - TFT_XMAX - 1);
        _curr_endp = constrain(_curr_endp, TFT_XMAX - L_EDGE, _data.getHeadCount() - 1);

        if (prev_startp != _curr_startp) {
            updateCurve();
            updateAxises();
            updateTicks();
        }
    }
}

template <typename input_type>
void Graph<input_type>::dynamicCursor(int8_t step) {

}

template <typename input_type>
void Graph<input_type>::annotate() {
    
}

template <typename input_type>
void Graph<input_type>::staticGraphCore(int16_t endp, bool local_sizing) {
    int16_t startp;

    _curr_endp = endp;
    _curr_startp = startp = endp + L_EDGE - TFT_XMAX;
    if (!local_sizing) {
        endp = _data.getHeadCount() - 1;
        startp = 0;
    }
    _curr_max = _data.findSampleMax(startp, endp);
    _curr_min = _data.findSampleMin(startp, endp);
    findAxisLevel();
    
    _tft.fillScreen(0x0000);
    updateCurve(true);
    updateAxises(true);
    updateTicks(true);
}

template <typename input_type>
void Graph<input_type>::updateCurve(bool initial) {
    int16_t x = L_EDGE;

    if (initial) {
        if (_curr_min >= 0) {
            memset(_prev_values, BT_EDGE - UP_EDGE - 1, sizeof(_prev_values));
        }
        else if (_curr_max <= 0) {
            memset(_prev_values, 1, sizeof(_prev_values));
        }
        else memset(_prev_values, _curr_level - UP_EDGE, sizeof(_prev_values));
    }

    for (int16_t i = _curr_startp; i <= _curr_endp; i++) {
        uint8_t h = round(mapFloat(_data.getData()[i].value,
                                   _curr_min, _curr_max, BT_EDGE - UP_EDGE - 1, 1));
        int16_t diff = h - _prev_values[x - L_EDGE];
        _prev_values[x - L_EDGE] = h;

        if (diff == 0) {
            x++;
            continue;
        }
        if (diff > 0) diff++;
        else diff--;

        uint16_t primary_color, secondary_color;
        bool junction = false;

        if (_curr_min >= 0) primary_color = (diff > 0) ? 0x0000 : PLOT_COLOR;
        else if (_curr_max <= 0) primary_color = (diff > 0) ? PLOT_COLOR : 0x0000;
        else {
            if (diff > 0) {
                if (h + UP_EDGE > _curr_level && h - diff + UP_EDGE >= _curr_level) {
                    primary_color = PLOT_COLOR;
                }
                else if (h + UP_EDGE > _curr_level && h - diff + UP_EDGE < _curr_level) {
                    junction = true;
                    primary_color = PLOT_COLOR; secondary_color = 0x0000;
                }
                else primary_color = 0x0000;
            }
            else {
                if (h + UP_EDGE >= _curr_level && h - diff + UP_EDGE > _curr_level) {
                    primary_color = 0x0000;
                }
                else if (h + UP_EDGE < _curr_level && h - diff + UP_EDGE > _curr_level) {
                    junction = true;
                    primary_color = PLOT_COLOR; secondary_color = 0x0000;
                }
                else primary_color = PLOT_COLOR;
            }
        }

        if (junction) {
            _tft.drawFastVLine(x, UP_EDGE + h, _curr_level - h - UP_EDGE, primary_color);
            _tft.drawFastVLine(x, _curr_level, UP_EDGE + h - diff - _curr_level, secondary_color);
        }
        else _tft.drawFastVLine(x, UP_EDGE + h, -diff, primary_color);

        x++;
    }
}

template <typename input_type>
void Graph<input_type>::updateAxises(bool initial) {
    _tft.drawFastHLine(L_EDGE - TICK_LEN, _curr_level, TFT_XMAX - L_EDGE + TICK_LEN, AXIS_COLOR);

    if (initial) {
        _tft.drawFastVLine(L_EDGE - 1, UP_EDGE - TICK_LEN, BT_EDGE - UP_EDGE + TICK_LEN, AXIS_COLOR);
        if (_curr_level != BT_EDGE) {
            _tft.drawFastHLine(L_EDGE - TICK_LEN, BT_EDGE, TFT_XMAX - L_EDGE + TICK_LEN, AXIS_COLOR);
        }
    }
}

template <typename input_type>
void Graph<input_type>::updateTicks(bool initial) {
    _tft.setTextColor(TEXT_COLOR);

    if (initial) memset(_tick_indexes, -1, sizeof(_tick_indexes));
    else {
        for (uint8_t i = 0; i * TICK_PER < 24; i++) {
            if (_tick_indexes[i] != -1 && _tick_indexes[i] < TFT_XMAX - 15) {
                _tft.drawFastVLine(_tick_indexes[i], BT_EDGE + 1, TICK_LEN, 0x0000);
                _tft.fillRect(_tick_indexes[i] - 14, BT_EDGE + 7, 31, 15, 0x0000);
                _tick_indexes[i] = -1;
            }
        }
    }

    for (uint8_t i = 0; i * TICK_PER < 24; i++) {
        uint8_t tick = i * TICK_PER;
        int8_t min_diff = 31;
        for (uint16_t j = _curr_startp; j <= _curr_endp; j++) {
            if (_data.getData()[j].hour == tick) {
                int8_t diff = min((int8_t)_data.getData()[j].minute, int8_t(60 - _data.getData()[j].minute));
                if (diff < min_diff) {
                    min_diff = diff;
                    _tick_indexes[i] = j - _curr_startp + L_EDGE;
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
void Graph<input_type>::findAxisLevel() {
    if (_curr_min >= 0) _curr_level = BT_EDGE;
    else if (_curr_max <= 0) _curr_level = UP_EDGE;
    else _curr_level = round(mapFloat(0, _curr_min, _curr_max, BT_EDGE, UP_EDGE));
}

template <typename input_type>
float Graph<input_type>::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}