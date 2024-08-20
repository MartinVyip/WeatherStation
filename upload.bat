@echo off
setlocal
set UPLOAD_PATH=C:\PlatformIO\Projects\WeatherStation\.pio\build\custom_board\firmware.bin
STM32_Programmer_CLI -c port=COM10 -w %UPLOAD_PATH% 0x08000000 -v