#!/usr/bin/env python3
"""
Basic LoRa Transmitter (TX) Example using pyradiolib on Raspberry Pi.
"""
import time
import sys
from pyradiolib import SX1262, ERR_NONE, get_status_text

def main():
    print("=" * 45)
    print(" pyradiolib - SX1262 Transmitter Example")
    print("=" * 45)

    # 1. Create the radio instance
    # SPI Channel 1 (CE1), NSS=7, DIO1=17, RESET=22, BUSY=24
    radio = SX1262(
        spi_channel=1,
        spi_speed=2000000,
        spi_device=0,
        gpio_device=0,
        nss=7,
        dio1=17,
        reset=22,
        busy=24
    )

    # 2. Initialize the radio for LoRa
    print("Initializing SX1262 ... ", end="", flush=True)
    status = radio.begin(
        frequency=866.5,       # Carrier frequency in MHz
        bandwidth=125.0,       # Bandwidth in kHz
        spreading_factor=7,    # Spreading factor (SF7)
        coding_rate=5,         # Coding rate 4/5
        sync_word=0x12,        # LoRa private sync word
        power=10,              # Output power in dBm
        preamble_length=8,     # Preamble length
        tcxo_voltage=1.6,      # TCXO reference voltage (set 0.0 if using crystal XTAL)
        use_regulator_ldo=False
    )

    if status != ERR_NONE:
        print(f"FAILED!\nError: {get_status_text(status)}")
        sys.exit(1)

    print("SUCCESS!")
    print("Transmitting packets every 2 seconds. Press Ctrl+C to exit.\n")

    counter = 0
    try:
        while True:
            message = f"Hello from Raspberry Pi LoRa #{counter}"
            print(f"[TX] Sending: \"{message}\" ... ", end="", flush=True)

            status = radio.transmit(message)
            if status == ERR_NONE:
                print("done!")
            else:
                print(f"FAILED! Error: {get_status_text(status)}")

            counter += 1
            time.sleep(2.0)

    except KeyboardInterrupt:
        print("\nStopping transmitter...")
    finally:
        radio.close()
        print("Radio resources released cleanly.")

if __name__ == "__main__":
    main()
