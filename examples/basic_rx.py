#!/usr/bin/env python3
"""
Basic LoRa Receiver (RX) Example using pyradiolib on Raspberry Pi.
"""
import sys
from pyradiolib import SX1262, ERR_NONE, ERR_RX_TIMEOUT, get_status_text

def main():
    print("=" * 45)
    print(" pyradiolib - SX1262 Receiver Example")
    print("=" * 45)

    # 1. Create the radio instance
    radio = SX1262(
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
    print("Initializing SX1262 ... ", end="", flush=True)
    status = radio.begin(
        frequency=866.5,
        bandwidth=125.0,
        spreading_factor=7,
        coding_rate=5,
        sync_word=0x12,
        power=10,
        preamble_length=8,
        tcxo_voltage=1.6,
        use_regulator_ldo=False
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
                    print(f"\n[RX] Receive status: {radio.get_status_text()}")

    except KeyboardInterrupt:
        print("\nStopping receiver...")
    finally:
        radio.close()
        print("Radio resources released cleanly.")

if __name__ == "__main__":
    main()
