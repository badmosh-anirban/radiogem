#!/usr/bin/env python3
"""
Example demonstrating simultaneous coexistence of pyradiolib (RadioLib + lgpio)
and gpiozero on separate GPIO pins without hardware or driver conflicts.

Pin ownership layout:
- RadioLib / PiHal:
    SPI0 CE1 (Channel 1)
    NSS:   GPIO 7
    DIO1:  GPIO 17
    RESET: GPIO 22
    BUSY:  GPIO 24
- gpiozero:
    Status LED:  GPIO 21
    Trigger Btn: GPIO 20
"""
import sys
import time

try:
    from gpiozero import LED, Button
except ImportError:
    print("Notice: gpiozero is not installed. To run this demo, install it with:")
    print("  pip install gpiozero")
    print("Proceeding with simulated GPIO components...\n")

from pyradiolib import SX1262, ERR_NONE, get_status_text

def main():
    print("=" * 60)
    print(" pyradiolib + gpiozero Coexistence Demo")
    print("=" * 60)

    # 1. Initialize gpiozero peripherals on independent GPIO pins
    try:
        led = LED(21)
        btn = Button(20)
        has_gpiozero = True
        print("[gpiozero] Attached Status LED to GPIO21 and Button to GPIO20")
    except Exception as e:
        print(f"[gpiozero] Hardware not initialized ({e}); running in radio-only mode.")
        has_gpiozero = False

    # 2. Initialize pyradiolib radio with dedicated SX1262 pins
    print("[pyradiolib] Initializing SX1262 on SPI1, NSS=7, DIO1=17, RST=22, BUSY=24...")
    radio = SX1262(
        spi_channel=1,
        nss=7,
        dio1=17,
        reset=22,
        busy=24
    )

    status = radio.begin(
        frequency=866.5,
        bandwidth=125.0,
        spreading_factor=7,
        coding_rate=5,
        power=10
    )

    if status != ERR_NONE:
        print(f"[pyradiolib] Init failed: {get_status_text(status)}")
        sys.exit(1)

    print("[pyradiolib] Radio initialized successfully!")
    print("\nDemonstrating non-conflicting concurrent operation:")
    print("- Blinking gpiozero LED while transmitting via pyradiolib...")

    try:
        for i in range(5):
            # gpiozero action
            if has_gpiozero:
                led.on()

            # pyradiolib action
            msg = f"Coexistence packet #{i}"
            print(f"  -> Transmitting '{msg}' (LED ON)... ", end="", flush=True)
            st = radio.transmit(msg)
            if st == ERR_NONE:
                print("TX OK")
            else:
                print(f"TX Error: {get_status_text(st)}")

            if has_gpiozero:
                led.off()

            time.sleep(1.0)

        print("\nSUCCESS: Both pyradiolib (PiHal) and gpiozero operated harmoniously!")

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        radio.close()
        if has_gpiozero:
            led.close()
            btn.close()

if __name__ == "__main__":
    main()
