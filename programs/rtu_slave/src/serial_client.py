#!/usr/bin/python3

import serial
import sys
import time
import os

msg = b'\x0a\x01\x27\x11\x00\x08\x66\x06'
msg = b'\x01\x03\x03\x00\x00\x01\x84\x4e'

def format(binary):
    return ' '.join(['0x%02x'%b for b in binary])

def main():
    # os.system('stty -hup  -F /dev/ttyACM0');
    ser = serial.Serial(
        '/dev/ttyACM0',
        115200,
        timeout=5,
        dsrdtr=False,
        parity=serial.PARITY_NONE
    )
    while True:
        time.sleep(2)
        ser.write(msg)
        line = ser.read(400)
        print(line)
        print(format(line))
        print(line.decode('utf-8', 'ignore'))
        print

if __name__ == '__main__':
    main()
