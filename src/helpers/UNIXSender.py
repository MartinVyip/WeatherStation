import serial
import keyboard
import time
from threading import Thread

GMT_OFFSET = +2
COM_PORT = 'COM10'
BAUD_RATE = 115200

trig = False

def press_check():
    global trig
    while not trig:
        if keyboard.is_pressed('esc'): trig = True

if __name__ == '__main__':
    checker = Thread(target=press_check)
    checker.start()
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        unix_time = int(time.time())
        while not trig:
            if int(time.time()) > unix_time:
                time_bytes = (unix_time + GMT_OFFSET * 3600).to_bytes(4, byteorder='big', signed=False)
                ser.write(time_bytes)
                print(f'Sent Unix time: {unix_time}')
            unix_time = int(time.time())
            time.sleep(0.001)
    except serial.SerialException as e:
        print(f'Error {e}')
        trig = True