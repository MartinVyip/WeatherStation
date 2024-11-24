#include <GyverPower.h>
#include <GyverBME280.h>
#include <SPI.h>
#include <RF24.h>
#define CE 7  //PD7
#define CSN 8  //PB0
#define BME_SUP PD2
#define NRF_SUP PD3

GyverBME280 sens;
RF24 radio(CE, CSN);

inline void sensInvoke() {
  sens.setMode(FORCED_MODE);
  sens.begin(0x76);
  sens.oneMeasurement();
}

inline void radioInvoke() {
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(0x60);
  radio.openWritingPipe(0x7878787878LL);
}

void setup() {
  power.autoCalibrate();
  power.setSleepMode(POWERDOWN_SLEEP);
  power.hardwareDisable(PWR_ADC | PWR_TIMER1 | PWR_TIMER2 | PWR_UART0 | PWR_UART1 | PWR_UART2 | PWR_UART3 | PWR_USB);
  power.setSystemPrescaler(PRESCALER_2);
}

void loop() {
  static float gauges[3];

  DDRD |= (1 << BME_SUP);
  DDRD |= (1 << NRF_SUP);
  PORTD |= (1 << BME_SUP);
  PORTD |= (1 << NRF_SUP);
  delay(10);
  
  sensInvoke();
  radioInvoke();
  while(sens.isMeasuring());
  gauges[0] = sens.readTemperature();
  gauges[1] = sens.readHumidity();
  gauges[2] = sens.readPressure();
  radio.write(&gauges,sizeof(gauges));
  
  PORTD &= ~(1 << BME_SUP);
  PORTD &= ~(1 << NRF_SUP);
  DDRD &= ~(1 << BME_SUP);
  DDRD &= ~(1 << NRF_SUP);
  power.sleepDelay(120000);
}