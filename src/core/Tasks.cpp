#include <config/Globals.h>
#include <core/Tasks.h>
#include <core/Backup.h>
#include <core/Display.h>
#include <core/Time.h>

void pollPower(void*) {
    for (;;) {
        if (!digitalRead(POW)) {
            digitalWrite(TFT_LED, LOW);
            for (int8_t i = 0; i < NUM_TASKS; i++) {
                if (tasks[i] != NULL && i != POWER_TASK) {
                    vTaskSuspend(tasks[i]);
                }
            }
            xSemaphoreGive(power_loss);
            xSemaphoreGive(vault_lock);
            while(!digitalRead(POW)) vTaskDelay(pdMS_TO_TICKS(POLL_POW_PER));
            xTaskCreate(emergencyBackup, "EmergencyBackup", 1024, NULL, 4, NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_POW_PER));
    }
}

void pollEncoder(void*) {
    for (;;) {
        enc.tick();
        if (enc.turn() || enc.busy()) {
            xSemaphoreGive(enc_event);
            xSemaphoreTake(enc_release, portMAX_DELAY);
        } else {
            vTaskDelay(pdMS_TO_TICKS(POLL_ENC_PER));
        }
    }
}

void pollRTCEvents(void*) {
    for (;;) {
        if (xSemaphoreTake(state_lock, portMAX_DELAY)) {
            if (state.curr_screen == MAIN) {
                uint8_t minute = rtc.getMinutes();
                if (minute != state.curr_mint) {
                    state.curr_mint = minute;
                    updateRTCEvents(state, minute);
                }
                updateConnectionStatus(state);
            }
            xSemaphoreGive(state_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_RTC_PER));
    }
}

void pollPIREvents(void*) {
    uint32_t prev_pir_signal = millis();
    for (;;) {
        uint32_t curr_time = millis();
        if (digitalRead(TFT_LED) && curr_time - prev_pir_signal > AWAKE_PER) {
            digitalWrite(TFT_LED, LOW);
            if (xSemaphoreTake(state_lock, portMAX_DELAY)) {
                if (state.curr_screen != MAIN) {
                    state.curr_screen = MAIN;
                    state.curr_mode = SCROLLING;
                    state.setup = true;
                }
                xSemaphoreGive(state_lock);
            }
        } else if (digitalRead(PIR) || enc.turn() || enc.busy()) {
            digitalWrite(TFT_LED, HIGH);
            prev_pir_signal = curr_time;
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_PIR_PER));
    }
}

void pollInputBuffers(void*) {
    vTaskDelay(pdMS_TO_TICKS(POLL_BUFS_PER));
    for (;;) {
        if (UART.available()) adjustRTC();
        if (radio.available()) {
            digitalWrite(LED, HIGH);
            float received_data[3];
            radio.read(&received_data, sizeof(received_data));
            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                out_temp.appendToAverage(received_data[0]);
                out_hum.appendToAverage(received_data[1]);
                out_press.appendToAverage(toMmHg(received_data[2]));
                xSemaphoreGive(vault_lock);
            }

            if (xSemaphoreTake(state_lock, portMAX_DELAY)) {
                state.prev_conn = millis();
                adjustSummertemp(&state.summertemp, received_data[0]);
                if (state.curr_screen == MAIN) {
                    updateIndicator(received_data[0], out_temp_ind, false);
                    updateIndicator(received_data[1], out_hum_ind, false);
                    updateIndicator(toMmHg(received_data[2]), out_press_ind, false);
                }
                xSemaphoreGive(state_lock);
            }
            digitalWrite(LED, LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_BUFS_PER));
    }
}

void emergencyBackup(void*) {
    if (xSemaphoreTake(power_loss, portMAX_DELAY)) {
        digitalWrite(LED, HIGH);
        if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
            finalizeBackup();
            xSemaphoreGive(vault_lock);
        }
        for (int8_t i = 0; i < NUM_TASKS; i++) {
            if (tasks[i] != NULL && i != POWER_TASK) {
                vTaskResume(tasks[i]);
            }
        }
        digitalWrite(LED, LOW);
    }
}

void periodicBackup(void*) {
    vTaskDelay(pdMS_TO_TICKS(STORE_PER));
    TickType_t last_wakeup = xTaskGetTickCount();
    for (;;) {
        if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
            createRawBackup();
            xSemaphoreGive(vault_lock);
        }
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(STORE_PER));
    }
}

void dataAppend(void*) {
    vTaskDelay(pdMS_TO_TICKS(APD_PER));
    TickType_t last_wakeup = xTaskGetTickCount();
    for (;;) {
        uint8_t weekday = rtc.getWeekDay() - 1;
        uint8_t hour = rtc.getHours();
        uint8_t minute = rtc.getMinutes();

        if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
            for (auto& vault : vaults) {
                vault->appendToVault(weekday, hour, minute);
            }
            if (xSemaphoreTake(state_lock, portMAX_DELAY)) {
                if (state.curr_screen == MAIN) {
                    int8_t rate = findWeatherRating(out_press.findNormalizedTrendSlope(BACKSTEP_PER),
                                                    out_hum.findNormalizedTrendSlope(BACKSTEP_PER),
                                                    out_temp.findNormalizedTrendSlope(BACKSTEP_PER));
                    xSemaphoreGive(vault_lock);
                    updateWeatherIcon(rate, state, false);
                }
                xSemaphoreGive(vault_lock);
                xSemaphoreGive(state_lock);
            }
        }
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(APD_PER));
    }
}

void dataUpdate(void*) {
    vTaskDelay(pdMS_TO_TICKS(UPD_PER));
    TickType_t last_wakeup = xTaskGetTickCount();
    for (;;) {
        float temp = bme.readTemperature();
        float hum = bme.readHumidity();
        uint16_t ppm = mhz.readCO2(true);

        if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
            in_temp.appendToAverage(temp);
            in_hum.appendToAverage(hum);
            co2_rate.appendToAverage(ppm);
            xSemaphoreGive(vault_lock);
        }

        if (xSemaphoreTake(state_lock, portMAX_DELAY)) {
            if (state.curr_screen == MAIN) {
                updateIndicator(temp, in_temp_ind, false);
                updateIndicator(hum, in_hum_ind, false);
                updateIndicator(ppm, co2_rate_ind, false);
            }
            xSemaphoreGive(state_lock);
        }
        digitalWrite(LED, LOW);
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(UPD_PER));
        digitalWrite(LED, HIGH);
    }
}

void plotUpdate(void*) {
    TickType_t last_wakeup = xTaskGetTickCount();
    for (;;) {
        if (xSemaphoreTake(state_lock, portMAX_DELAY)) {
            uint32_t curr_time = millis();
            uint16_t curr_head_count;
            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                curr_head_count = out_temp.getHeadCount();
                xSemaphoreGive(vault_lock);
            }

            switch (state.curr_mode) {
                case SCROLLING: {
                    if (state.setup) {
                        state.setup = false;
                        tft.fillScreen(0x0000);
                        if (state.curr_screen != MAIN) {
                            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                                switch (state.curr_screen) {
                                    case OUT_TEMP: plot = new Graph<float>(out_temp, tft); break;
                                    case OUT_HUM: plot = new Graph<float>(out_hum, tft); break;
                                    case OUT_PRESS: plot = new Graph<float>(out_press, tft); break;
                                    case IN_TEMP: plot = new Graph<float>(in_temp, tft); break;
                                    case IN_HUM: plot = new Graph<float>(in_hum, tft); break;
                                    case CO2_RATE: plot = new Graph<uint16_t>(co2_rate, tft); break;
                                }
                                plot->drawFresh();
                                plot->drawLogos(state.curr_screen, state.curr_mint);
                                plot->annotate();
                                xSemaphoreGive(vault_lock);
                            }
                        } else {
                            buildMainScreen(state);
                            state.curr_mint = rtc.getMinutes();
                        }
                    }

                    if (xSemaphoreTake(enc_event, 0)) {
                        if (enc.turn() && curr_head_count) {
                            if (enc.left()) {
                                state.curr_screen = (state.curr_screen > MAIN) ?
                                                    (screens)(state.curr_screen - 1) : CO2_RATE;
                            } else if (enc.right()) {
                                state.curr_screen = (state.curr_screen < CO2_RATE) ?
                                                    (screens)(state.curr_screen + 1) : MAIN;
                            }
                            delete plot;
                            plot = nullptr;
                            state.setup = true;
                        } else if (enc.click()) {
                            if (state.curr_screen != MAIN && curr_head_count > TFT_XMAX - L_EDGE) {
                                state.curr_mode = PANNING;
                                state.setup = true;
                            } else if (state.curr_screen != MAIN && curr_head_count > CRECT_SIDE) {
                                state.curr_mode = CURSOR;
                                state.setup = true;
                            }
                        }
                        xSemaphoreGive(enc_release);
                    }
                }
                break;
                case PANNING: {
                    if (state.setup) {
                        state.setup = false;
                        if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                            plot->drawLocal(false);
                            plot->drawLogos(state.curr_screen, state.summertemp);
                            plot->annotate();
                            xSemaphoreGive(vault_lock);
                        }
                    }

                    if (xSemaphoreTake(enc_event, 0)) {
                        if (enc.turn()) {
                            int8_t step = ((enc.fast()) ? PAN_FAST : PAN_SLOW) * enc.dir();
                            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                                plot->dynamicPan(step);
                                xSemaphoreGive(vault_lock);
                            }
                        } else if (enc.click()) {
                            state.curr_mode = CURSOR;
                            state.setup = true;
                        } else if (enc.hold()) {
                            while(enc.holding()) enc.tick();
                            state.curr_mode = SCROLLING;
                            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                                plot->drawLocal();
                                plot->drawLogos(state.curr_screen, state.summertemp);
                                plot->annotate();
                                xSemaphoreGive(vault_lock);
                            }
                        }
                        xSemaphoreGive(enc_release);
                    }
                }
                break;
                case CURSOR: {
                    if (state.setup) {
                        state.setup = false;
                        if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                            plot->drawLocal();
                            plot->drawLogos(state.curr_screen, state.summertemp);
                            plot->annotate(false);
                            plot->drawCursor(true);
                            xSemaphoreGive(vault_lock);
                        }
                    }

                    if (xSemaphoreTake(enc_event, 0)) {
                        if (enc.turn()) {
                            int8_t step = ((enc.fast()) ? CRSR_FAST : CRSR_SLOW) * enc.dir();
                            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                                plot->dynamicCursor(step);
                                xSemaphoreGive(vault_lock);
                            }
                        } else if (enc.click() && curr_head_count > TFT_XMAX - L_EDGE) {
                            state.curr_mode = PANNING;
                            state.setup = true;
                        } else if (enc.hold() || enc.click()) {
                            while(enc.holding()) enc.tick();
                            state.curr_mode = SCROLLING;
                            if (xSemaphoreTake(vault_lock, portMAX_DELAY)) {
                                plot->drawLocal();
                                plot->drawLogos(state.curr_screen, state.summertemp);
                                plot->annotate();
                                xSemaphoreGive(vault_lock);
                            }
                        }
                        xSemaphoreGive(enc_release);
                    }
                }
                break;
            }
            xSemaphoreGive(state_lock);
        }
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(SCREEN_UPD_PER));
    }
}