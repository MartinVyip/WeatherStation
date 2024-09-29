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
void Graph<input_type>::drawCursor(bool initial) {
    if (_data.getHeadCount() <= CRECT_SIDE) return;
    if (initial) _curr_index = (TFT_XMAX - L_EDGE) >> 1;

    drawCursorPointer();
    drawCursorData();
}

template <typename input_type>
void Graph<input_type>::dynamicPan(int8_t step) {
    if (_data.getHeadCount() <= TFT_XMAX - L_EDGE) return;
    int16_t prev_startp = _curr_startp;

    _curr_startp += step;
    _curr_endp += step;
    _curr_startp = constrain(_curr_startp, 0, _data.getHeadCount() + L_EDGE - TFT_XMAX - 1);
    _curr_endp = constrain(_curr_endp, TFT_XMAX - L_EDGE, _data.getHeadCount() - 1);

    if (prev_startp != _curr_startp) {
        updateCurve();
        updateAxises();
        updateTicks();
        updateWeekdays();
    }
}

template <typename input_type>
void Graph<input_type>::dynamicCursor(int8_t step) {
    _prev_index = _curr_index;
    _curr_index += step;
    _curr_index = constrain(_curr_index, CRECT_HALF, TFT_XMAX - L_EDGE - CRECT_HALF);

    if (_prev_index != _curr_index) {
        erasePrevCursor();
        if (abs(_prev_values[_prev_index] - BT_EDGE + UP_EDGE) <= CRECT_HALF) {
            updateAxises(true);
            updateTicks();
        } else updateAxises();
        drawCursor();
    }
}

template <typename input_type>
void Graph<input_type>::annotate(bool dayscale) {
    if (dayscale) {
        updateWeekdays(true);
    }
    _tft.setTextColor(TEXT_CLR4);
    _tft.setTextSize(1);
    _tft.setFont(&CustomFont10pt);

    char max[10], min[10];
    _data.getCharValue(_curr_max, max);
    _data.getCharValue(_curr_min, min);
    _tft.setCursor(5, UP_EDGE + 40);
    _tft.print(max);
    _tft.setCursor(5, BT_EDGE - 15);
    _tft.print(min);

    uint16_t width = getTextWidth(max, _tft);
    _tft.drawFastHLine(L_EDGE - 4, UP_EDGE, CRECT_HALF, LINK_CLR);
    _tft.drawFastHLine(5, UP_EDGE + 45, 2 + width, LINK_CLR);
    _tft.drawLine(7 + width, UP_EDGE + 45, L_EDGE - 4, UP_EDGE, LINK_CLR);

    width = getTextWidth(min, _tft);
    _tft.drawLine(7 + width, BT_EDGE - 10, L_EDGE - CRECT_HALF, BT_EDGE, LINK_CLR);
    _tft.drawFastHLine(5, BT_EDGE - 10, 2 + width, LINK_CLR);
}

template <typename input_type>
void Graph<input_type>::drawLogos(enum screens screen, bool high) {
    if (high) {
        _tft.drawRGBBitmap(5, 5, high_bitmaps[screen], 60, 60);
    } else {
        _tft.drawRGBBitmap(5, 5, low_bitmaps[screen], 60, 60);
    }
    _tft.drawRGBBitmap(10, 130, tal_tech, 50, 30);
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
        } else if (_curr_max <= 0) {
            memset(_prev_values, 1, sizeof(_prev_values));
        } else memset(_prev_values, _curr_level - UP_EDGE, sizeof(_prev_values));
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

        if (_curr_min >= 0) primary_color = (diff > 0) ? 0x0000 : PLOT_CLR;
        else if (_curr_max <= 0) primary_color = (diff > 0) ? PLOT_CLR : 0x0000;
        else {
            if (diff > 0) {
                if (h + UP_EDGE > _curr_level && h - diff + UP_EDGE >= _curr_level) {
                    primary_color = PLOT_CLR;
                } else if (h + UP_EDGE > _curr_level && h - diff + UP_EDGE < _curr_level) {
                    junction = true;
                    primary_color = PLOT_CLR; secondary_color = 0x0000;
                } else primary_color = 0x0000;
            } else {
                if (h + UP_EDGE >= _curr_level && h - diff + UP_EDGE > _curr_level) {
                    primary_color = 0x0000;
                } else if (h + UP_EDGE < _curr_level && h - diff + UP_EDGE > _curr_level) {
                    junction = true;
                    primary_color = PLOT_CLR; secondary_color = 0x0000;
                } else primary_color = PLOT_CLR;
            }
        }

        if (junction) {
            _tft.drawFastVLine(x, UP_EDGE + h, _curr_level - h - UP_EDGE, primary_color);
            _tft.drawFastVLine(x, _curr_level, UP_EDGE + h - diff - _curr_level, secondary_color);
        } else _tft.drawFastVLine(x, UP_EDGE + h, -diff, primary_color);

        x++;
    }
}

template <typename input_type>
void Graph<input_type>::updateAxises(bool initial) {
    _tft.drawFastHLine(L_EDGE - CRECT_HALF, _curr_level, TFT_XMAX - L_EDGE + CRECT_HALF, AXIS_CLR);

    if (initial) {
        _tft.drawFastVLine(L_EDGE - 1, UP_EDGE - TICK_LEN, BT_EDGE - UP_EDGE + TICK_LEN, AXIS_CLR);
        if (_curr_level != BT_EDGE) {
            _tft.drawFastHLine(L_EDGE - CRECT_HALF, BT_EDGE, TFT_XMAX - L_EDGE + CRECT_HALF, AXIS_CLR);
        }
    }
}

template <typename input_type>
void Graph<input_type>::updateTicks(bool initial) {
    _tft.setTextColor(TEXT_CLR1);
    _tft.setTextSize(1);
    _tft.setFont();

    if (initial) memset(_tick_posns, -1, sizeof(_tick_posns));
    else {
        for (uint8_t i = 0; i * TICK_PER < 24; i++) {
            if (_tick_posns[i] != -1 && _tick_posns[i] < TFT_XMAX - 15) {
                _tft.drawFastVLine(_tick_posns[i], BT_EDGE + 1, TICK_LEN, 0x0000);
                _tft.fillRect(_tick_posns[i] - 14, BT_EDGE + 7, 31, 15, 0x0000);
                _tick_posns[i] = -1;
            }
        }
    }

    for (uint8_t i = 0; i * TICK_PER < 24; i++) {
        uint8_t tick = i * TICK_PER;
        for (uint16_t j = _curr_startp; j <= _curr_endp; j++) {
            if (j == 0) continue;
            if (_data.getData()[j].hour == tick) {
                int8_t diff = min((int8_t)_data.getData()[j].minute,
                                  int8_t(60 - _data.getData()[j - 1].minute));
                _tick_posns[i] = j - _curr_startp + L_EDGE;
                if (diff != _data.getData()[j].minute) _tick_posns[i]--;
                break;
            }
        }
        if (_tick_posns[i] != -1 && _tick_posns[i] < TFT_XMAX - 15) {
            char hours[2];
            _tft.drawFastVLine(_tick_posns[i], BT_EDGE, TICK_LEN, TICK_CLR);
            itoa(tick, hours, DEC);
            _tft.setCursor(_tick_posns[i] - 6 * strlen(hours) - 2, BT_EDGE + 7);
            _tft.print(hours);
            _tft.setCursor(_tick_posns[i] + 4, BT_EDGE + 7);
            _tft.print("00");
        }
    }
}

template <typename input_type>
void Graph<input_type>::updateWeekdays(bool initial) {
    _tft.setTextColor(TEXT_CLR2);
    _tft.setTextSize(1);
    _tft.setFont(&CustomFont10pt);

    if (initial) {
        _tft.fillRect(L_EDGE - 1, UP_EDGE - 14, TFT_XMAX - L_EDGE, 2, SEP_CLR);
        _tft.drawFastVLine(L_EDGE - 1, 0, UP_EDGE - TICK_LEN, AXIS_CLR);
    } else {
        _tft.fillRect(_separtr_index, UP_EDGE - 15, 2, -SEP_LEN, 0x0000);
        _tft.fillRect(_spot_posns[0], UP_EDGE - 40, 19 * _spot_lengths[0], 20, 0x0000);
        _tft.fillRect(_spot_posns[1], UP_EDGE - 40, 19 * _spot_lengths[1], 20, 0x0000);
    }

    for (uint16_t i = _curr_startp; i <= _curr_endp; i++) {
        if (i == 0) continue;
        if (_data.getData()[i].weekday != _data.getData()[i - 1].weekday) {
            _separtr_index = i - _curr_startp + L_EDGE;
            break;
        }
    }

    _tft.fillRect(_separtr_index, UP_EDGE - 15, 2, -SEP_LEN, SEP_CLR);
    if (_separtr_index < TFT_XMAX - 60) {
        _spot_lengths[0] = constrain((TFT_XMAX - _separtr_index) / 25, 3,
                                      strlen(weekdays[_data.getData()[_curr_endp].weekday]));
        _spot_posns[0] = ((_separtr_index + TFT_XMAX) >> 1) - ((16 * _spot_lengths[0]) >> 1);
        _spot_posns[0] = constrain(_spot_posns[0], L_EDGE, TFT_XMAX);
        _tft.setCursor(_spot_posns[0], UP_EDGE - 25);
        _tft.write(weekdays[_data.getData()[_curr_endp].weekday], _spot_lengths[0]);
    }
    if (_separtr_index > L_EDGE + 60) {
        _spot_lengths[1] = constrain((_separtr_index - L_EDGE) / 25, 3,
                                      strlen(weekdays[_data.getData()[_curr_startp].weekday]));
        _spot_posns[1] = ((_separtr_index + L_EDGE) >> 1) - ((16 * _spot_lengths[1]) >> 1);
        _spot_posns[1] = constrain(_spot_posns[1], L_EDGE, TFT_XMAX);
        _tft.setCursor(_spot_posns[1], UP_EDGE - 25);
        _tft.write(weekdays[_data.getData()[_curr_startp].weekday], _spot_lengths[1]);
    }
}

template <typename input_type>
void Graph<input_type>::drawCursorPointer() {
    _cursor_x = _curr_index + L_EDGE;
    uint16_t rect_x = _cursor_x - CRECT_HALF;
    uint16_t rect_y = _prev_values[_curr_index] + UP_EDGE - CRECT_HALF;

    _tft.drawRect(rect_x, rect_y, CRECT_SIDE, CRECT_SIDE, CRSR_CLR);
    _tft.drawRect(rect_x + 1, rect_y + 1, CRECT_SIDE - 2, CRECT_SIDE - 2, CRSR_CLR);
    _tft.drawFastVLine(_cursor_x, UP_EDGE - CRECT_SIDE, BT_EDGE - UP_EDGE + CRECT_SIDE, CRSR_CLR);
}

template <typename input_type>
void Graph<input_type>::drawCursorData() {
    char time[6], value[10];

    _data.getCharTime(_curr_startp + _curr_index, time);
    _data.getCharValue(_data.getData()[_curr_startp + _curr_index].value, value);
    uint8_t time_len = strlen(time);
    uint8_t value_len = strlen(value);
    _window_width = 6 * max(time_len, value_len) + 8;
    uint16_t window_half = _window_width >> 1;

    if (TFT_XMAX - _cursor_x < window_half) {
        _tft.drawRoundRect(TFT_XMAX - _window_width, 5, _window_width, 30, 3, CRSR_CLR);
        _cursor_x = TFT_XMAX - window_half;
    } else if (_cursor_x - L_EDGE < window_half) {
        _tft.drawRoundRect(L_EDGE, 5, _window_width, 30, 3, CRSR_CLR);
        _cursor_x = L_EDGE + window_half;
    } else {
        _tft.drawRoundRect(_cursor_x - window_half, 5, _window_width, 30, 3, CRSR_CLR);
    }

    _tft.setTextColor(TEXT_CLR3);
    _tft.setTextSize(1);
    _tft.setFont();
    _tft.setCursor(_cursor_x - 3 * time_len, 10);
    _tft.print(time);
    _tft.setCursor(_cursor_x - 3 * value_len, 22);
    _tft.print(value);
}

template <typename input_type>
void Graph<input_type>::erasePrevCursor() {
    eraseCursorRect();
    eraseCursorLine();
    eraseCursorData();
}

template <typename input_type>
void Graph<input_type>::eraseCursorRect() {
    int16_t x = L_EDGE + _prev_index - CRECT_HALF;
    int16_t prev_val_lower = _prev_values[_prev_index] - CRECT_HALF;
    int16_t prev_val_upper = _prev_values[_prev_index] + CRECT_HALF;
    int16_t curr_relative_level = _curr_level - UP_EDGE;

    for (uint16_t i = x - L_EDGE; i <= _prev_index + CRECT_HALF; i++) {
        int16_t prev_val = _prev_values[i];
        int16_t vline_start, vline_height;
        uint16_t color = 0x0000;

        if (prev_val_upper < curr_relative_level) {
            if (prev_val_lower > prev_val) {
                vline_start = prev_val_lower + UP_EDGE - 1;
                vline_height = CRECT_SIDE + 1;
                color = PLOT_CLR;
            } else if (prev_val_upper < prev_val) {
                vline_start = prev_val_lower + UP_EDGE - 1;
                vline_height = CRECT_SIDE + 1;
            } else {
                _tft.drawFastVLine(x, prev_val + UP_EDGE, prev_val_upper - prev_val + 1, PLOT_CLR);
                _tft.drawFastVLine(x, prev_val + UP_EDGE, prev_val_lower - prev_val - 1, color);
                x++;
                continue;
            }
        } else if (prev_val_lower > curr_relative_level) {
            if (prev_val_lower > prev_val) {
                vline_start = prev_val_lower + UP_EDGE - 1;
                vline_height = CRECT_SIDE + 1;
            } else if (prev_val_upper < prev_val) {
                vline_start = prev_val_lower + UP_EDGE - 1;
                vline_height = CRECT_SIDE + 1;
                color = PLOT_CLR;
            } else {
                _tft.drawFastVLine(x, prev_val + UP_EDGE, prev_val_upper - prev_val + 1, color);
                _tft.drawFastVLine(x, prev_val + UP_EDGE, prev_val_lower - prev_val - 1, PLOT_CLR);
                x++;
                continue;
            }
        } else {
            int16_t temp = prev_val_lower + UP_EDGE;
            if (prev_val_lower > prev_val) {
                _tft.drawFastVLine(x, temp - 1, _curr_level - temp + 1, PLOT_CLR);
                _tft.drawFastVLine(x, _curr_level, temp + 1 + CRECT_SIDE - _curr_level, color);
            } else if (prev_val_upper < prev_val) {
                _tft.drawFastVLine(x, temp - 1, _curr_level - temp + 1, color);
                _tft.drawFastVLine(x, _curr_level, temp + 1 + CRECT_SIDE - _curr_level, PLOT_CLR);
            } else if (prev_val <= curr_relative_level) {
                _tft.drawFastVLine(x, prev_val + UP_EDGE, curr_relative_level - prev_val, PLOT_CLR);
                _tft.drawFastVLine(x, prev_val + UP_EDGE, prev_val_lower - prev_val - 1, color);
                _tft.drawFastVLine(x, _curr_level, prev_val_upper - _curr_level + UP_EDGE + 1, color);
            } else {
                _tft.drawFastVLine(x, prev_val + UP_EDGE, curr_relative_level - prev_val, PLOT_CLR);
                _tft.drawFastVLine(x, prev_val + UP_EDGE, prev_val_upper - prev_val + 1, color);
                _tft.drawFastVLine(x, _curr_level, prev_val_lower - _curr_level + UP_EDGE - 1, color);
            }
            x++;
            continue;
        }

        _tft.drawFastVLine(x, vline_start, vline_height, color);
        x++;
    }
}

template <typename input_type>
void Graph<input_type>::eraseCursorLine() {
    int16_t x = L_EDGE + _prev_index;
    int16_t y = _prev_values[_prev_index] + UP_EDGE;

    if (_curr_min >= 0) {
        _tft.drawFastVLine(x, y, BT_EDGE - y, PLOT_CLR);
        _tft.drawFastVLine(x, y, UP_EDGE - CRECT_SIDE - y - 1, 0x0000);
    } else if (_curr_max <= 0) {
        _tft.drawFastVLine(x, y, BT_EDGE - y, 0x0000);
        _tft.drawFastVLine(x, _curr_level, -CRECT_SIDE - 1, 0x0000);
        _tft.drawFastVLine(x, y, UP_EDGE - y - 1, PLOT_CLR);
    } else {
        _tft.drawFastVLine(x, y, _curr_level - y, PLOT_CLR);
        if (y < _curr_level) {
            _tft.drawFastVLine(x, y, UP_EDGE - CRECT_SIDE - y - 1, 0x0000);
            _tft.drawFastVLine(x, _curr_level, BT_EDGE - _curr_level, 0x0000);
        } else {
            _tft.drawFastVLine(x, y, BT_EDGE - y, 0x0000);
            _tft.drawFastVLine(x, _curr_level, UP_EDGE - CRECT_SIDE - _curr_level - 1, 0x0000);
        }
    }
}

template <typename input_type>
void Graph<input_type>::eraseCursorData() {
    _tft.fillRoundRect(_cursor_x - (_window_width >> 1), 5, _window_width, 30, 3, 0x0000);
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

template <typename input_type>
uint16_t Graph<input_type>::getTextWidth(const char* string, Adafruit_ILI9341& tft_reference) {
    int16_t x1, y1;
    uint16_t h, width;
    tft_reference.getTextBounds(string, 0, 0, &x1, &y1, &width, &h);
    return width;
}