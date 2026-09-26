#!/usr/bin/env python3
"""
Example demonstrating changing radio and LoRa modulation settings at runtime
using pyradiolib (analogous to SX126x_Settings.ino in C++).
"""
import sys
from pyradiolib import (
    SX1262,
    ERR_NONE,
    get_status_text,
    ERR_INVALID_BANDWIDTH,
    ERR_INVALID_SPREADING_FACTOR,
    ERR_INVALID_CODING_RATE,
    ERR_INVALID_OUTPUT_POWER,
    ERR_INVALID_CURRENT_LIMIT
)

def check_status(action_name, status):
    if status == ERR_NONE:
        print(f"  [OK] {action_name}")
    else:
        print(f"  [FAIL] {action_name} failed: {get_status_text(status)}")
        sys.exit(1)

def main():
    print("=" * 60)
    print(" pyradiolib - Runtime Settings Example")
    print("=" * 60)

    radio = SX1262(
        spi_channel=1,
        nss=7,
        dio1=17,
        reset=22,
        busy=24
    )

    print("Initializing radio with base configuration...")
    st = radio.begin(frequency=866.5, bandwidth=125.0, spreading_factor=7, coding_rate=5)
    check_status("Initial begin()", st)

    print("\nModifying radio settings dynamically at runtime:")

    # 1. Change carrier frequency
    st = radio.set_frequency(868.0)
    check_status("Set frequency to 868.0 MHz", st)

    # 2. Change bandwidth (125.0, 250.0, 500.0 kHz, etc.)
    st = radio.set_bandwidth(250.0)
    check_status("Set bandwidth to 250.0 kHz", st)

    # 3. Change spreading factor (5 to 12)
    st = radio.set_spreading_factor(9)
    check_status("Set spreading factor to SF9", st)

    # 4. Change coding rate (5 to 8 for 4/5 to 4/8)
    st = radio.set_coding_rate(7)
    check_status("Set coding rate to 4/7", st)

    # 5. Set LoRa sync word (e.g. 0x12 for private networks, 0x34 for public/LoRaWAN)
    st = radio.set_sync_word(0x12)
    check_status("Set LoRa sync word to 0x12", st)

    # 6. Change output power (-9 to +22 dBm)
    st = radio.set_output_power(14)
    check_status("Set output power to 14 dBm", st)

    # 7. Set over-current protection limit (45 - 240 mA, or 0 to disable)
    st = radio.set_current_limit(80.0)
    check_status("Set current limit to 80 mA", st)

    # 8. Set LoRa preamble length (0 to 65535 symbols)
    st = radio.set_preamble_length(12)
    check_status("Set preamble length to 12 symbols", st)

    # 9. Configure CRC checksum
    st = radio.set_crc(True)
    check_status("Enable CRC (2-byte checksum)", st)

    # 10. Configure TCXO voltage if supported by board (e.g. 1.6V to 3.3V, or 0.0 for XTAL)
    st = radio.set_tcxo(1.6)
    check_status("Set TCXO to 1.6V", st)

    # 11. Configure DIO2 as RF switch control
    st = radio.set_dio2_as_rf_switch(True)
    check_status("Configure DIO2 as RF switch", st)

    print("\nSUCCESS: All runtime settings applied and validated successfully!")
    radio.close()

if __name__ == "__main__":
    main()
