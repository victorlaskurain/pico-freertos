#!/usr/bin/python3

import logging
logging.basicConfig()
log = logging.getLogger()
log.setLevel(logging.INFO)

from pymodbus.client.sync import ModbusSerialClient
from pymodbus import utilities
import os
import time

def main():
    client = ModbusSerialClient(
        method = "rtu",
        port="/dev/ttyACM0",
        stopbits = 1,
        bytesize = 8,
        parity = 'N',
        baudrate= 115200,
        dsrdtr=False,
        timeout=0.01)
    conn = client.connect()

    print('client.read_holding_registers(0x0000, 1, unit=0x01)')
    registers = client.read_holding_registers(0x0300, 6, unit=0x01)
    print(registers)
    print(registers.registers)

if __name__ == '__main__':
    main()
