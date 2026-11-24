#!/usr/bin/env python3
"""
Basic LoRa Receiver (RX) Example for SX1278 using radiogem on Raspberry Pi.
SX1278 is a 1st generation Semtech LoRa transceiver (137-525 MHz, default: 434 MHz).
"""
import sys
import argparse
from radiogem import SX1278, ERR_NONE, ERR_RX_TIMEOUT, get_status_text

def main():
    parser = argparse.ArgumentParser(description="radiogem SX1278 Receiver Example")
    parser.add_argument("--freq", type=float, default=434.0, help="Frequency in MHz (default: 434.0)")
    parser.add_argument("--nss", type=int, default=7, help="NSS / CS GPIO pin (default: 7)")
    parser.add_argument("--dio0", type=int, default=25, help="DIO0 IRQ GPIO pin (default: 25)")
    parser.add_argument("--reset", type=int, default=22, help="Reset GPIO pin (default: 22)")
    args = parser.parse_args()

    print("=" * 45)
    print(" radiogem - SX1278 Receiver Example")
    print(f" Frequency: {args.freq} MHz")
    print(f" Pins: NSS={args.nss} DIO0={args.dio0} RESET={args.reset}")
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
        power=10,              # Output power in dBm
        preamble_length=8,     # Preamble length in symbols
        gain=0                 # 0 = AGC automatic gain control
    )

    if status != ERR_NONE:
        print(f"FAILED!\nError: {get_status_text(status)}")
        sys.exit(1)

    print("SUCCESS!")
    print("Listening for incoming packets. Press Ctrl+C to exit.\n")

    try:
        while True:
            # timeout_ms=5000: wait up to 5 seconds for a packet
            packet = radio.receive(timeout_ms=5000)

            if packet:
                print(f"[RX] Received Packet ({len(packet)} bytes):")
                print(f"     Payload: \"{packet.text}\"")
                print(f"     RSSI:    {packet.rssi:.1f} dBm")
                print(f"     SNR:     {packet.snr:.1f} dB")
            else:
                # Timed out or CRC error
                if radio.last_status == ERR_RX_TIMEOUT:
                    print(".", end="", flush=True)
                else:
                    print(f"\n[RX] Receive error: {get_status_text(radio.last_status)}")

    except KeyboardInterrupt:
        print("\nStopping receiver...")
    finally:
        radio.close()
        print("Radio resources freed.")

if __name__ == "__main__":
    main()

