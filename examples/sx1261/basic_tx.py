#!/usr/bin/env python3
"""
Basic LoRa Transmitter (TX) Example for SX1261 using radiogem on Raspberry Pi.
SX1261 features high-efficiency power amplifier supporting -17 dBm to +15 dBm.
"""
import time
import sys
import argparse
from radiogem import SX1261, ERR_NONE, get_status_text

def main():
    parser = argparse.ArgumentParser(description="radiogem SX1261 Transmitter Example")
    parser.add_argument("--interval", type=float, default=1.0, help="Transmit interval in seconds")
    parser.add_argument("--freq", type=float, default=866.5, help="Frequency in MHz (default: 866.5)")
    parser.add_argument("--power", type=int, default=14, help="TX power in dBm (-17 to +15, default: 14)")
    parser.add_argument("--tcxo", type=float, default=1.6, help="TCXO voltage (set 0.0 for XTAL crystals)")
    args = parser.parse_args()

    print("=" * 45)
    print(" radiogem - SX1261 Transmitter Example")
    print(f" Frequency: {args.freq} MHz | TX Power: {args.power} dBm")
    print(f" TX Interval: {args.interval}s")
    print("=" * 45)

    # 1. Create the radio instance
    radio = SX1261(
        spi_channel=1,
        spi_speed=2000000,
        spi_device=0,
        gpio_device=0,
        nss=7,
        dio1=27,
        reset=22,
        busy=24
    )

    # 2. Initialize the radio for LoRa
    print("Initializing SX1261 ... ", end="", flush=True)
    status = radio.begin(
        frequency=args.freq,   # Carrier frequency in MHz
        bandwidth=125.0,       # Bandwidth in kHz
        spreading_factor=7,    # Spreading factor (SF7)
        coding_rate=5,         # Coding rate 4/5
        sync_word=0x12,        # LoRa private sync word
        power=args.power,      # Output power in dBm (-17 to +15 dBm for SX1261)
        preamble_length=8,     # Preamble length
        tcxo_voltage=args.tcxo,# TCXO reference voltage (0.0 if using crystal XTAL)
        use_regulator_ldo=False
    )

    if status != ERR_NONE:
        print(f"FAILED!\nError: {get_status_text(status)}")
        sys.exit(1)

    print("SUCCESS!")
    print(f"Transmitting packets every {args.interval}s. Press Ctrl+C to exit.\n")

    counter = 0
    try:
        while True:
            counter += 1
            payload = f"Hello from SX1261 #{counter}"
            print(f"[{counter:04d}] Transmitting: \"{payload}\" ... ", end="", flush=True)

            t_start = time.time()
            tx_status = radio.transmit(payload)
            t_duration = (time.time() - t_start) * 1000

            if tx_status == ERR_NONE:
                print(f"OK ({t_duration:.1f}ms)")
            else:
                print(f"FAILED! Error: {get_status_text(tx_status)}")

            time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\nStopping transmitter...")
    finally:
        radio.close()
        print("Radio resources freed.")

if __name__ == "__main__":
    main()

