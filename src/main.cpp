#include <config/Globals.h>
#include <core/Backup.h>
#include <core/Display.h>
#include <core/Peripherals.h>
#include <core/Time.h>
#include <core/Tasks.h>

SPIClass tftSPI(TFT_MOSI, TFT_MISO, TFT_CLK);
TwoWire I2C(SDA, SCL);
HardwareSerial UART(RX, TX);

Adafruit_ILI9341 tft(&tftSPI, TFT_DC, TFT_CS, TFT_RESET);
BME280 bme(0x76, &I2C);
MHZ19B mhz(MH_PWM, MH_HD);
EncButton enc(ENC_S1, ENC_S2, ENC_KEY);
RF24 radio(RF_CE, RF_CSN);
I2C_eeprom eeprom(0x50, I2C_DEVICESIZE_24LC256, &I2C);
STM32RTC& rtc = STM32RTC::getInstance();

DataVault <float> out_temp(TEMP_NORM_RANGE, eeprom);
DataVault <float> out_hum(HUM_NORM_RANGE, eeprom);
DataVault <float> out_press(PRESS_NORM_RANGE, eeprom);
DataVault <float> in_temp(eeprom);
DataVault <float> in_hum(eeprom);
DataVault <uint16_t> co2_rate(eeprom);

GraphBase* plot = nullptr;
VaultBase* vaults[] = {
    &out_temp, &out_hum, &out_press,
    &in_temp, &in_hum,
    &co2_rate
};

uint16_t last_day_min;
bool backup_ready = false;

state_config state;
SemaphoreHandle_t power_loss, enc_event, enc_release;
SemaphoreHandle_t state_lock, vault_lock;
TaskHandle_t tasks[12];


void setup() {
    hardwareSetup();

    if (READ_BACKUP_STATE()) {
        digitalWrite(LED, HIGH);
        tft.fillScreen(0x0000);
        pullBackup();
        digitalWrite(LED, LOW);
    }

    power_loss = xSemaphoreCreateBinary();
    enc_event = xSemaphoreCreateBinary();
    enc_release = xSemaphoreCreateBinary();

    state_lock = xSemaphoreCreateMutex();
    vault_lock = xSemaphoreCreateMutex();

    xTaskCreate(pollPower, "PowerPinPolling", 128, NULL, 4, &tasks[POWER_TASK]);
    xTaskCreate(pollEncoder, "EncoderPolling", 128, NULL, 4, &tasks[ENC_TASK]);
    xTaskCreate(pollRTCEvents, "RTCEvents", 256, NULL, 3, &tasks[RTC_TASK]);
    xTaskCreate(pollPIREvents, "PIRPolling", 128, NULL, 1, &tasks[PIR_TASK]);
    xTaskCreate(pollInputBuffers, "BuffersPolling", 128, NULL, 2, &tasks[BUFFERS_TASK]);

    xTaskCreate(periodicBackup, "PeriodicBackup", 1024, NULL, 2, &tasks[PERIODIC_BACKUP_TASK]);
    xTaskCreate(emergencyBackup, "EmergencyBackup", 1024, NULL, 4, NULL);

    xTaskCreate(dataAppend, "DataAppend", 256, NULL, 3, &tasks[APPEND_TASK]);
    xTaskCreate(dataUpdate, "DataUpdate", 256, NULL, 2, &tasks[UPDATE_TASK]);
    xTaskCreate(plotUpdate, "PlotUpdate", 1024, NULL, 4, &tasks[PLOT_TASK]);

    vTaskStartScheduler();
}


void loop() {}