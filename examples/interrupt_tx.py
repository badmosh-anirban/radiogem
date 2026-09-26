#!/usr/bin/env python3
"""
Non-blocking / Interrupt-driven LoRa Transmitter Example using pyradiolib.
Demonstrates:
  1. Starting transmission without blocking the Python thread (start_transmit)
  2. Waiting for transmission completion via callback (set_packet_sent_action)
  3. Proper cleanup after transmission finishes (finish_transmit)
"""
import time
import sys
from pyradiolib import SX1262, ERR_NONE, get_status_text

transmitted_flag = False

def main():
    print("=" * 60)
    print(" pyradiolib - Non-blocking Interrupt Transmitter Example")
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

    # Callback when DIO1 signals packet transmission completed
    def on_packet_sent():
        global transmitted_flag
        transmitted_flag = True

    radio.set_packet_sent_action(on_packet_sent)

    counter = 0
    try:
        while True:
            msg = f"Non-blocking packet #{counter}"
            print(f"[TX] Starting non-blocking send: \"{msg}\" ... ", end="", flush=True)

            transmitted_flag = False
            status = radio.start_transmit(msg)
            if status != ERR_NONE:
                print(f"FAILED to start! Error: {get_status_text(status)}")
            else:
                print("started!")

                # While the radio is transmitting over the air, Python can do other work:
                while not (transmitted_flag or radio.has_sent):
                    # Do background tasks, UI updates, sensor reads, etc.
                    time.sleep(0.005)

                # Transmission finished! Finish transmit to disable RF switch and enter standby
                radio.finish_transmit()
                print("     [TX Done] Packet was sent over the air successfully!")

            counter += 1
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        radio.clear_packet_sent_action()
        radio.close()
        print("Radio closed cleanly.")

if __name__ == "__main__":
    main()
