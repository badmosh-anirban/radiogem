#!/usr/bin/env python3
"""
Non-blocking / Interrupt-driven LoRa Receiver Example using pyradiolib.
Demonstrates:
  1. Callback-based interrupt reception (set_packet_received_action)
  2. Continuous listening mode without blocking timeouts (start_receive)
"""
import time
import sys
from pyradiolib import SX1262, ERR_NONE, get_status_text

# Flag to signal that a packet has arrived
received_flag = False

def main():
    global received_flag
    print("=" * 60)
    print(" pyradiolib - Non-blocking Interrupt Receiver Example")
    print("=" * 60)

    radio = SX1262(
        spi_channel=1,
        nss=7,
        dio1=17,
        reset=22,
        busy=24
    )

    print("Initializing SX1262 ... ", end="", flush=True)
    status = radio.begin(
        frequency=866.5,
        bandwidth=125.0,
        spreading_factor=7,
        coding_rate=5,
        sync_word=0x12,
        power=10,
        tcxo_voltage=1.6
    )

    if status != ERR_NONE:
        print(f"FAILED!\nError: {get_status_text(status)}")
        sys.exit(1)

    print("SUCCESS!")

    # Define the callback function that will be executed when an interrupt fires on DIO1
    def on_packet_received():
        global received_flag
        received_flag = True

    # Register the callback with the radio interrupt system
    radio.set_packet_received_action(on_packet_received)

    # Start listening in continuous receive mode (no timeouts!)
    print("Starting continuous receive mode ... ", end="", flush=True)
    status = radio.start_receive()
    if status != ERR_NONE:
        print(f"FAILED!\nError: {get_status_text(status)}")
        sys.exit(1)

    print("SUCCESS!")
    print("Radio is listening in the background. Press Ctrl+C to exit.\n")

    try:
        while True:
            # Check either our Python flag or the radio's internal atomic flag:
            if received_flag or radio.has_received:
                received_flag = False

                # Read the packet data
                packet = radio.read_data()
                if packet:
                    print(f"[INTERRUPT RX] Packet ({len(packet)} bytes):")
                    print(f"   Payload: \"{packet.text}\"")
                    print(f"   RSSI:    {packet.rssi:.1f} dBm")
                    print(f"   SNR:     {packet.snr:.1f} dB")
                else:
                    print(f"[INTERRUPT RX] Read error: {radio.get_status_text()}")

                # Put the radio back into continuous listen mode for the next packet
                radio.start_receive()

            # The main thread is completely free to perform other work!
            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        radio.clear_packet_received_action()
        radio.close()
        print("Radio closed cleanly.")

if __name__ == "__main__":
    main()
