# Meteostation Project

A comprehensive weather monitoring system tracking both indoor and outdoor parameters. Capable of storing and graphically respresenting weather data. Performs short-term weather prediction and data restoration in case of emergency power off.


## Project Structure

```plaintext
hardware/
├── BaseStation/
│   ├── Schematic_BaseStation.pdf    # Circuit design for Base Station
│   ├── 3Dmodels/
│   │   ├── .sldprt/                 # SolidWorks parts
│   │   └── .stl/                    # Printable 3D files
│   └── PCB/
│       ├── BOM.csv                  # Bill of materials
│       ├── Gerber files             # Manufacturing-ready files
│       └── Layout PDFs              # Top/Bottom views
├── SensModule/
│   ├── Schematic_SensingModule.pdf  # Circuit design for the module
│   ├── 3Dmodels/                    # 3D models of the module
│   └── PCB/                         # PCB designs
include/
├── classes/
│   ├── DataVault.h                  # Data storage and retrieval
│   └── GraphingEngine.h             # Graphical representation of data
├── config/
│   ├── Constants.h                  # IO pins, macros, settings etc.
│   └── Enums&Structs.h              # Data structure definitions
├── rsc/
│   ├── Bitmaps.h                    # Bitmap graphics
│   └── fonts/                       # Custom fonts
└── utils/
    ├── SolarWeatherUtils.h          # Solar events and weather estimation
    └── TimeUtils.h                  # Time-related utilities
logos/
├── indoor_indicator.png             # Icon for indoor parameters
├── tal_tech.png                     # Branding
├── connection_icons/                # Icons representing connection status
├── parameter_icons/                 # Icons for weather metrics
└── weather_icons/                   # Icons for weather conditions
src/
├── main.cpp                         # Main station program
├── classes/                         # Class implementations
├── helpers/
│   └── UNIXSender.py                # Script for setting time 
└── utils/                           # Utility implementations
src_module/
└── main.ino                         # Main sensor module program
platformio.ini                       # PlatformIO configuration file
upload.bat                           # Booting script
```


## Station

### **Basic Fucntion**
The central hub of the meteostation, responsible for managing data and interfacing with the sensor module. It retrieves environmental data every two minutes, stores it, and updates a corresponding indicator. Every six minutes, it averages incoming values and saves them to a vault.

Main screen shows:
- **Indoor**: Temperature, humidity, and CO₂ level.
- **Outdoor**: Temperature, humidity, and pressure.
- Time, date, weekday.
- Connection status icon.
- Short-term weather prediction.

### **Time Setting**
The time is automatically adjusted for [DST](https://en.wikipedia.org/wiki/Daylight_saving_time) and can also be precisely set using a Python script that sends [UNIX time](https://en.wikipedia.org/wiki/Unix_time) to the station via UART, requiring a computer connection.

### **Weather Prediction**
Weather prediction is performed based on [least squares interpolation](https://en.wikipedia.org/wiki/Simple_linear_regression) of temperature, humidity, and pressure trends over the past 1.5 hours. The prediction uses a [complementary filter](https://www.sciencedirect.com/topics/computer-science/complementary-filter) to combine trends and determine the weather condition, displayed as a dynamic weather icon (e.g., sunny, stormy).

### **Day/Night Calculation**
**Day and night icons** are adjusted based on sunrise and sunset times, calculated daily using the following method:
1. **Solar Noon Approximation**:  
   The solar noon time is estimated using the cosine function to account for seasonal variations in daylight length based on the current day of the year.

2. **Correction Using the Equation of Time**:  
   The solar noon estimate is refined with the [equation of time](https://en.wikipedia.org/wiki/Equation_of_time), which accounts for the irregularities caused by the Earth's orbital eccentricity and axial tilt.

Using the corrected solar noon as a reference, the station calculates sunrise and sunset times by incorporating the device’s latitude, [GMT offset](https://en.wikipedia.org/wiki/Greenwich_Mean_Time), and the declination of the Sun. This method achieves a precision of a few minutes.

### **Custom Graphics** 
A custom font, designed with the [Adafruit GFX Font Customiser](https://tchapi.github.io/Adafruit-GFX-Font-Customiser/), ensures clarity. All icons were converted to bitmaps using the [Bitmaper application](https://alexgyver.github.io/Bitmaper/).

### **Connection Status Icon**
The connection icon provides real-time feedback:  
- **Bright white**: Connection active.
- **Dim white**: Pending.
- **Off**: Connection lost (pending for 5 minutes).

### **User Interaction**
A PIR sensor detects user presence, automatically turning off the display backlight after a minute of inactivity.

### **Graphical Data Analysis** 
The base station stores a week of weather data, allowing users to study metrics graphically. Graphs display daily spans, automatically scaled with maximum and minimum indicators. A rotary encoder enables scrolling through week-long data, switching between day-based and week-based views, and zooming in with a cursor to inspect specific data points. All graph dynamics are rendered with real-time internal graphics calculations, leveraging the STM32F4's [floating-point unit](https://en.wikipedia.org/wiki/Floating-point_unit) for glitch-free performance.

### **Power Loss Recovery**
In case of a power loss, a 0.1F supercapacitor allows data to be backed up to 32kB EEPROM. The station performs periodic raw data backups every hour, saving only a portion of data directly during power loss. Upon restoration, the device fills gaps using the last available value and recalculates time offsets. A hard reset button clears all stored data, while an RTC powered by a 25F supercapacitor ensures accurate timekeeping.


## SensModule

### **Basic Fucntion**
The remote sensor module collects outdoor environmental data and transmits it to the base station via a radio connection. The module uses a PCB antenna, while the station features a 3dBi antenna with an SMA connector for enhanced signal strength.

### **Power Sources** 
The module is powered by a combination of a solar panel with a 25F supercapacitor and a 3V lithium battery. The solar panel serves as the primary power source, with the lithium battery as a backup for extreme conditions, such as prolonged snow coverage.

### **Delivering Power**
A single-chip boost converter has been added to step up the voltage from the power sources to a stable 3.5V. This ensures reliable operation even with low input voltage (as low as 400mV) from the solar panel. [Power supply redundancy schematic](https://elentec.narod.ru/Documents/part10/Index0.htm) prioritizes the higher voltage source, allowing the module to operate seamlessly under varying conditions.

### **Low Power Consumption**
The module is exceptionally efficient, consuming just 5.8µA in idle mode (99.5% of the time). This allows the lithium battery to power the module for over four years without solar input.