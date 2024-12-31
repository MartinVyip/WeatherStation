#include <config/Globals.h>
#include <core/Display.h>

void clearRectangle(const icon_config& icon) {
    tft.fillRect(icon.x, icon.y, icon.width, icon.height, 0x0000);
}

void drawIcon(const icon_config& icon) {
    tft.drawRGBBitmap(icon.x, icon.y, icon.bitmap, icon.width, icon.height);
}

void updateTime(uint8_t minute) {
    char timestring[5];
    sprintf(timestring, "%02d:%02d", rtc.getHours(), minute);
    updateIndicator(timestring, time_ind, false);
}

void updateDate() {
    char datestring[10];
    sprintf(datestring, "%02d.%02d.%d", rtc.getDay(), rtc.getMonth(), rtc.getYear());
    updateIndicator(datestring, date_ind, false);
}

void updateWeatherIcon(int8_t weather_rating, state_config& state, bool initial) {
    static const icon_config* optimal = nullptr;

    if (initial && optimal != nullptr) {
        drawIcon(*optimal);
    }

    if (weather_rating >= 0) {
        for (const auto& config : positive_weathers) {
            if (weather_rating >= config.min_rating && weather_rating <= config.max_rating) {
                optimal = state.daytime ? &config.option1 : &config.option2;
                clearRectangle(weather_icon);
                drawIcon(*optimal);
                break;
            }
        }
    } else {
        for (const auto& config : negative_weathers) {
            if (weather_rating >= config.min_rating && weather_rating <= config.max_rating) {
                optimal = state.summertemp ? &config.option1 : &config.option2;
                clearRectangle(weather_icon);
                drawIcon(*optimal);
                break;
            }
        }
    }
}

void updateConnectionIcon(enum conn_statuses connection_status, bool initial) {
    if (!initial) clearRectangle(link_icon);
    const uint16_t* status_bitmap = (connection_status == RECEIVING) ? receiving
                                  : (connection_status == NO_CONN) ? no_connect
                                  : pending;
    icon_config status_icon = {status_bitmap, link_icon.x, link_icon.y,
                               link_icon.width, link_icon.height};
    drawIcon(status_icon);
}

void updateConnectionStatus(state_config& state) {
    uint32_t curr_time = millis();
    conn_statuses new_status = (curr_time - state.prev_conn < RECEIVE_THRES) ? RECEIVING
                             : (curr_time - state.prev_conn >= PENDING_THRES) ? NO_CONN
                             : PENDING;

    if (new_status != state.radio_status) {
        state.radio_status = new_status;
        if (state.curr_screen == MAIN) {
            updateConnectionIcon(state.radio_status, false);
        }
    }
}

void buildMainScreen(state_config& state) {
    drawIcon(indoor_icon);
    updateIndicator(out_temp.getLastValue(), out_temp_ind, true);
    updateIndicator(out_hum.getLastValue(), out_hum_ind, true);
    updateIndicator(out_press.getLastValue(), out_press_ind, true);
    updateIndicator(bme.readTemperature(), in_temp_ind, true);
    updateIndicator(bme.readHumidity(), in_hum_ind, true);
    updateIndicator(mhz.readCO2(false), co2_rate_ind, true);
    updateIndicator(weekdays[rtc.getWeekDay() - 1], weekday_ind, true);

    int8_t rate = findWeatherRating(out_press.findNormalizedTrendSlope(BACKSTEP_PER),
                                    out_hum.findNormalizedTrendSlope(BACKSTEP_PER),
                                    out_temp.findNormalizedTrendSlope(BACKSTEP_PER));
    updateWeatherIcon(rate, state, true);
    updateConnectionIcon(state.radio_status, true);
    updateTime(rtc.getMinutes());
    updateDate();
}