#!/usr/bin/env python3
"""
Basic LoRa Transmitter (TX) Example for SX1278 using radiogem on Raspberry Pi.
SX1278 is a 1st generation Semtech LoRa transceiver (137-525 MHz, default: 434 MHz).
"""
import time
import sys
import argparse
from radiogem import SX1278, ERR_NONE, get_status_text

def main():
    parser = argparse.ArgumentParser(description="radiogem SX1278 Transmitter Example")
    parser.add_argument("--interval", type=float, default=1.0, help="Transmit interval in seconds (default: 1.0s)")
    parser.add_argument("--freq", type=float, default=434.0, help="Frequency in MHz (default: 434.0)")
    parser.add_argument("--power", type=int, default=10, help="TX power in dBm (2 to 17/20, default: 10)")
    parser.add_argument("--nss", type=int, default=7, help="NSS / CS GPIO pin (default: 7)")
    parser.add_argument("--dio0", type=int, default=25, help="DIO0 IRQ GPIO pin (default: 25)")
    parser.add_argument("--reset", type=int, default=22, help="Reset GPIO pin (default: 22)")
    args = parser.parse_args()

    print("=" * 45)
    print(" radiogem - SX1278 Transmitter Example")
    print(f" Frequency: {args.freq} MHz | TX Power: {args.power} dBm")
    print(f" Pins: NSS={args.nss} DIO0={args.dio0} RESET={args.reset}")
    print(f" TX Interval: {args.interval}s")
    print("=" * 45)

    # 1. Create the radio instance
    radio = SX1278(
        spi_channel=1,
        spi_speed=2000000,
        spi_device=0,
        gpio_device=0,
        nss=args.nss,
        dio0=args.dio0,
        reset=args.reset
    )

    # 2. Initialize the radio for LoRa
    print("Initializing SX1278 ... ", end="", flush=True)
    status = radio.begin(
        frequency=args.freq,   # Carrier frequency in MHz (434.0 MHz)
        bandwidth=125.0,       # Bandwidth in kHz
        spreading_factor=7,    # Spreading factor (SF7)
        coding_rate=5,         # Coding rate 4/5
        sync_word=0x12,        # LoRa private sync word
        power=args.power,      # Output power in dBm (2 to 17 dBm)
        preamble_length=8,     # Preamble length in symbols
        gain=0                 # 0 = AGC automatic gain control
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
            payload = f"Hello from SX1278 #{counter}"
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

