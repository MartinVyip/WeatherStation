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
DataVault <uint16_t> out_press(PRESS_NORM_RANGE, eeprom);
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
TaskHandle_t task_handles[12];
TaskHandle_t blink_task = NULL;


void setup() {
    hardwareSetup();

    if (READ_BACKUP_STATE()) {
        tft.fillScreen(0x0000);
        pullBackup();
    }

    power_loss = xSemaphoreCreateBinary();
    enc_event = xSemaphoreCreateBinary();
    enc_release = xSemaphoreCreateBinary();

    state_lock = xSemaphoreCreateMutex();
    vault_lock = xSemaphoreCreateMutex();

    xTaskCreate(pollPower, "PowerPinPolling", 128, NULL, 4, &task_handles[POWER_TASK]);
    xTaskCreate(pollEncoder, "EncoderPolling", 128, NULL, 4, &task_handles[ENC_TASK]);
    xTaskCreate(pollRTCEvents, "RTCEvents", 256, NULL, 3, &task_handles[RTC_TASK]);
    xTaskCreate(pollPIREvents, "PIRPolling", 128, NULL, 1, &task_handles[PIR_TASK]);
    xTaskCreate(pollInputBuffers, "BuffersPolling", 128, NULL, 2, &task_handles[BUFFERS_TASK]);

    xTaskCreate(emergencyBackup, "EmergencyBackup", 256, NULL, 4, NULL);
    xTaskCreate(periodicBackup, "PeriodicBackup", 256, NULL, 2, &task_handles[PERIODIC_BACKUP_TASK]);

    xTaskCreate(dataAppend, "DataAppend", 256, NULL, 3, &task_handles[APPEND_TASK]);
    xTaskCreate(dataUpdate, "DataUpdate", 256, NULL, 2, &task_handles[UPDATE_TASK]);
    xTaskCreate(plotUpdate, "PlotUpdate", 512, NULL, 4, &task_handles[PLOT_TASK]);

    vTaskStartScheduler();
}


void loop() {}