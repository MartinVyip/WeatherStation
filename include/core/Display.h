#ifndef Display_h
#define Display_h

#include <config/Globals.h>

void clearRectangle(const icon_config& icon);
void drawIcon(const icon_config& icon);

template <typename input_type>
void updateIndicator(input_type value, const indicator_config& settings, bool initial) {
    char output[15];

    if constexpr (std::is_arithmetic<input_type>::value) {
        DataVault<input_type>::getCharValue(value, output, settings.forced_round);
        strcat(output, settings.unit);
    } else {
        strcpy(output, static_cast<const char*>(value));
    }

    tft.setTextColor(settings.color);
    tft.setFont(settings.font);

    if (settings.alignment == "left") {
        tft.setCursor(settings.aln_x, settings.aln_y);
    } else {
        uint16_t width = Graph<input_type>::getTextWidth(output, tft);
        if (settings.alignment == "right") {
            tft.setCursor(settings.aln_x - width, settings.aln_y);
        } else if (settings.alignment == "center") {
            tft.setCursor(settings.aln_x - (width >> 1), settings.aln_y);
        }
    }

    if (!initial) {
        tft.fillRect(settings.bound_x, settings.bound_y,
                     settings.bound_width, settings.bound_height, 0x0000);
    }
    tft.write(output);
}

void updateTime(uint8_t minute);
void updateDate();
void updateWeatherIcon(int8_t weather_rating, state_config& state, bool initial);
void updateConnectionIcon(enum conn_statuses connection_status, bool initial);
void updateConnectionStatus(state_config& state);

void buildMainScreen(state_config& state);

#endif