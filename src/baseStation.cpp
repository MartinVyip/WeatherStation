#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RF24.h>
#include <SPI.h>
#include <STM32RTC.h>
#include <EncButton.h>
#include <I2C_eeprom.h>

// ==================== SETTINGS ====================
#define TEXT_COLOR 0xC618
#define AXIS_COLOR 0x07E0
#define TICK_COLOR 0x07E0
#define TICK_LEN 10
#define TICK_PER 6

// ==================== IO PINS ====================
#define LED PC6
#define TX PA9
#define RX PA10
#define SDA PB9
#define SCL PB8
#define TFT_MOSI PB15
#define TFT_MISO PB14  // NC
#define TFT_CLK PB13
#define TFT_CS PD2
#define TFT_DC PC11
#define TFT_RESET PC12
#define TFT_LED PC10
#define RF_CE PC4
#define RF_CSN PA4

SPIClass tftSPI(TFT_MOSI, TFT_MISO, TFT_CLK);
HardwareSerial UART(RX, TX);
TwoWire I2C(SDA, SCL);

Adafruit_ILI9341 tft(&tftSPI, TFT_DC, TFT_CS, TFT_RESET);
RF24 radio(RF_CE, RF_CSN);
I2C_eeprom eeprom(0x50, &I2C);
STM32RTC& rtc = STM32RTC::getInstance();

template <typename input_type>
class Graph {

// ==================== DEVELOPMENT CONSTANTS ====================
#define TFT_XMAX 320
#define TFT_YMAX 240
#define L_EDGE 80
#define UP_EDGE 50
#define BT_EDGE 220

// ==================== EXCEPTIONS ====================
#if (TICK_PER < 4 || 24 % TICK_PER != 0)
#error "Invalid ticking period"
#endif

#if (TICK_LEN <= 0 || TICK_LEN > TFT_YMAX - BT_EDGE)
#error "Invalid tick length"
#endif

    struct DataPoint {
        input_type value;
        uint8_t weekday;
        uint8_t hour;
        uint8_t minute;
    };

    public:
        Graph(uint16_t data_size) {
            _data = new DataPoint[data_size];
            _buffer_size = data_size;
            for (uint8_t i = 0; 24 * i < TICK_PER; i++) {
                _tick_indexes[i] = -1;
            }
        }

        ~Graph() {
            delete[] _data;
        }

        void appendValue(input_type value, uint8_t a, uint8_t b) {
            if (_head_count < _buffer_size) {
                _data[_head_count] = {value, 1, a, b};
                _head_count++;
            }
            else {
                for (uint16_t i = 1; i < _buffer_size; i++) {
                    _data[i - 1] = _data[i];
                }
                _data[_buffer_size - 1] = {value, 1, a, b};
            }
        }

        void drawStaticGraph(bool local_sizing = true, int16_t endp = INT16_MAX) {
            endp = (endp == INT16_MAX) ? (_head_count - 1) : endp;
            int16_t startp, graph_startp, graph_endp;

            tft.fillScreen(0x0000);
            endp = graph_endp = (endp < _buffer_size) ? endp : _buffer_size - 1;
            startp = graph_startp = (endp + L_EDGE - TFT_XMAX < 0) ? 0 : endp + L_EDGE - TFT_XMAX;
            if (!local_sizing) {
                endp = ((_head_count < _buffer_size) ? _head_count : _buffer_size) - 1;
                startp = 0;
            }
            input_type sample_max = findSampleMax(startp, endp);
            input_type sample_min = findSampleMin(startp, endp);
            
            uint8_t axis_level;
            if (sample_min >= 0) axis_level = BT_EDGE;
            else if (sample_max <= 0) axis_level = UP_EDGE;
            else axis_level = round(mapFloat(0, sample_min, sample_max, BT_EDGE - UP_EDGE, 0)) + UP_EDGE;
            
            for (uint16_t i = graph_startp; i <= graph_endp; i++) {
                uint16_t x = i - graph_startp + L_EDGE;
                uint8_t h = round(mapFloat(_data[i].value, sample_min, sample_max, BT_EDGE - UP_EDGE, 0));
                if (sample_min >= 0) {
                    tft.drawFastVLine(x, BT_EDGE, -h, 0xFFFF);
                }
                else if (sample_max <= 0) {
                    tft.drawFastVLine(x, UP_EDGE, h, 0xFFFF);
                }
                else tft.drawFastVLine(x, axis_level, h - axis_level + UP_EDGE, 0xFFFF);
            }
            drawAxises(axis_level);
            drawTicks(graph_startp, graph_endp);
        }

        void graphDynamicChange(int8_t step) {

        }

    private:
        DataPoint* _data;
        uint16_t _buffer_size;
        uint16_t _head_count;
        int16_t _tick_indexes[24 / TICK_PER];

        void drawAxises(uint8_t axis_level) {
            tft.drawFastVLine(L_EDGE, UP_EDGE - TICK_LEN, BT_EDGE - UP_EDGE + TICK_LEN, AXIS_COLOR);
            tft.drawFastHLine(L_EDGE - TICK_LEN, axis_level, TFT_XMAX - L_EDGE + TICK_LEN, AXIS_COLOR);
            if (axis_level != BT_EDGE) {
                tft.drawFastHLine(L_EDGE - TICK_LEN, BT_EDGE, TFT_XMAX - L_EDGE + TICK_LEN, AXIS_COLOR);
            }
        }

        void drawTicks(uint16_t startpoint, uint16_t endpoint) {
            tft.setTextColor(TEXT_COLOR);
            for (uint8_t i = 0; i * TICK_PER < 24; i++) {
                if (_tick_indexes[i] != -1) {
                    tft.drawFastVLine(_tick_indexes[i], BT_EDGE + 1, TICK_LEN, 0x0000);
                    tft.fillRect(_tick_indexes[i] - 14, BT_EDGE + 7, 31, 15, 0x0000);
                    _tick_indexes[i] = -1;
                }
            }
            for (uint8_t i = 0; i * TICK_PER < 24; i++) {
                uint8_t tick = i * TICK_PER;
                int8_t min_diff = 31;
                for (uint16_t j = startpoint; j <= endpoint; j++) {
                    if (_data[j].hour == tick) {
                        int8_t diff = min((int8_t)_data[j].minute, int8_t(60 - _data[j].minute));
                        if (diff < min_diff) {
                            min_diff = diff;
                            _tick_indexes[i] = j - startpoint + L_EDGE;
                        }
                    }
                }
                if (_tick_indexes[i] != -1 && _tick_indexes[i] < TFT_XMAX - 15) {
                    char hours[2];
                    tft.drawFastVLine(_tick_indexes[i], BT_EDGE, TICK_LEN, TICK_COLOR);
                    itoa(tick, hours, DEC);
                    tft.setCursor(_tick_indexes[i] - 6 * strlen(hours) - 2, BT_EDGE + 7);
                    tft.print(hours);
                    tft.setCursor(_tick_indexes[i] + 4, BT_EDGE + 7);
                    tft.print("00");
                }
            }
        }

        input_type findSampleMax(uint16_t startpoint, uint16_t endpoint) {
            input_type max_value = _data[startpoint].value;
            for (uint16_t i = startpoint + 1; i <= endpoint; i++) {
                max_value = max(max_value, _data[i].value);
            }
            return max_value;
        }

        input_type findSampleMin(uint16_t startpoint, uint16_t endpoint) {
            input_type min_value = _data[startpoint].value;
            for (uint16_t i = startpoint + 1; i < endpoint; i++) {
                min_value = min(min_value, _data[i].value);
            }
            return min_value;
        }

        float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
            return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        }
};

inline void tftSetup() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    tft.begin();
    tft.setRotation(3);
}

inline void radioSetup() {
    radio.begin();
    radio.setAutoAck(true);
    radio.setRetries(0, 5);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(0x60);
    radio.openReadingPipe(1, 0x7878787878LL);
    radio.startListening();
}

inline void rtcSetup() {
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
}

Graph <float> out_temp(1680);

void setup() {
    UART.begin(115200);
    I2C.begin();

    tftSetup();
    radioSetup();
    rtcSetup();
    eeprom.begin();

    UART.println("start");
}

void loop() {
    if (radio.available()) {
        float received_data[3];
        radio.read(&received_data, sizeof(received_data));
    }
    if (UART.available()) {
        uint8_t time_bytes[4];
        uint32_t unix_time;
        UART.readBytes(time_bytes, sizeof(time_bytes));
        unix_time = (uint32_t)time_bytes[0] << 24 |
                    (uint32_t)time_bytes[1] << 16 |
                    (uint32_t)time_bytes[2] << 8 |
                    (uint32_t)time_bytes[3];
        rtc.setEpoch(unix_time);
    }
    //UART.printf("%02d.%02d.%02d  ", rtc.getDay(), rtc.getMonth(), rtc.getYear());
    //UART.printf("%02d:%02d:%02d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
}