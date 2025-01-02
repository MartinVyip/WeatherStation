#ifndef Globals_h
#define Globals_h

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <SPI.h>
#include <Wire.h>

#include <config/Constants.h>
#include <classes/GraphingEngine.h>
#include <classes/DataVault.h>
#include <utils/BME280.h>
#include <utils/MHZ19B.h>
#include <utils/SolarWeatherUtils.h>
#include <utils/TimeUtils.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RF24.h>
#include <STM32RTC.h>
#include <EncButton.h>
#include <I2C_eeprom.h>

extern SPIClass tftSPI;
extern TwoWire I2C;
extern HardwareSerial UART;

extern Adafruit_ILI9341 tft;
extern BME280 bme;
extern MHZ19B mhz;
extern EncButton enc;
extern RF24 radio;
extern I2C_eeprom eeprom;
extern STM32RTC& rtc;

extern DataVault <float> out_temp;
extern DataVault <float> out_hum;
extern DataVault <uint16_t> out_press;
extern DataVault <float> in_temp;
extern DataVault <float> in_hum;
extern DataVault <uint16_t> co2_rate;

extern GraphBase* plot;
extern VaultBase* vaults[6];

extern uint16_t last_day_min;
extern bool backup_ready;

extern state_config state;
extern SemaphoreHandle_t power_loss, enc_event, enc_release;
extern SemaphoreHandle_t state_lock, vault_lock;
extern TaskHandle_t tasks[12];

#endif