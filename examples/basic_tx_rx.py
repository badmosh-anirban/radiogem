#!/usr/bin/env python3
"""
Bidirectional LoRa TX / RX Utility using pyradiolib on Raspberry Pi.
"""
import argparse
import time
import sys
from pyradiolib import SX1262, ERR_NONE, ERR_RX_TIMEOUT, get_status_text

def main():
    parser = argparse.ArgumentParser(description="RadioLib SX1262 LoRa Demo on Raspberry Pi")
    parser.add_argument("--mode", choices=["tx", "rx"], default="tx", help="Operation mode (tx or rx)")
    parser.add_argument("--freq", type=float, default=866.5, help="Frequency in MHz (default: 866.5)")
    parser.add_argument("--bw", type=float, default=125.0, help="Bandwidth in kHz (default: 125.0)")
    parser.add_argument("--sf", type=int, default=7, help="Spreading factor 5-12 (default: 7)")
    parser.add_argument("--cr", type=int, default=5, help="Coding rate 5-8 (default: 5)")
    parser.add_argument("--power", type=int, default=10, help="TX power in dBm (default: 10)")
    parser.add_argument("--spi-ch", type=int, default=1, help="SPI Channel (default: 1 for CE1)")
    parser.add_argument("--nss", type=int, default=7, help="NSS pin (default: 7)")
    parser.add_argument("--dio1", type=int, default=17, help="DIO1 pin (default: 17)")
    parser.add_argument("--rst", type=int, default=22, help="Reset pin (default: 22)")
    parser.add_argument("--busy", type=int, default=24, help="Busy pin (default: 24)")
    parser.add_argument("--tcxo", type=float, default=1.6, help="TCXO voltage (set 0.0 for XTAL crystals)")
    args = parser.parse_args()

    print("=" * 55)
    print(" pyradiolib SX1262 Interactive Utility")
    print(f" Mode: {args.mode.upper()} | Freq: {args.freq} MHz | SF{args.sf} | BW {args.bw} kHz")
    print(f" Pins: SPI_CH={args.spi_ch} NSS={args.nss} DIO1={args.dio1} RST={args.rst} BUSY={args.busy}")
    print("=" * 55)

    with SX1262(
        spi_channel=args.spi_ch,
        spi_speed=2000000,
        nss=args.nss,
        dio1=args.dio1,
        reset=args.rst,
        busy=args.busy
    ) as radio:

        print("Initializing radio modem ... ", end="", flush=True)
        status = radio.begin(
            frequency=args.freq,
            bandwidth=args.bw,
            spreading_factor=args.sf,
            coding_rate=args.cr,
            power=args.power,
            tcxo_voltage=args.tcxo
        )

        if status != ERR_NONE:
            print(f"FAILED!\nError: {get_status_text(status)}")
            sys.exit(1)

        print("SUCCESS!\n")

        if args.mode == "tx":
            counter = 0
            print("Starting transmission loop (Ctrl+C to exit)...")
            try:
                while True:
                    payload = f"RaspberryPi-LoRa-{counter}"
                    print(f"[TX #{counter}] Sending: '{payload}' ... ", end="", flush=True)
                    st = radio.transmit(payload)
                    if st == ERR_NONE:
                        print("OK")
                    else:
                        print(f"FAILED: {get_status_text(st)}")
                    counter += 1
                    time.sleep(2.0)
            except KeyboardInterrupt:
                print("\nTransmission stopped.")
        else:
            print("Listening for packets (Ctrl+C to exit)...")
            try:
                while True:
                    pkt = radio.receive(timeout_ms=5000)
                    if pkt:
                        print(f"[RX] Size: {len(pkt)} bytes | RSSI: {pkt.rssi:.1f} dBm | SNR: {pkt.snr:.1f} dB")
                        print(f"     Payload: \"{pkt.text}\"")
                    else:
                        if radio.last_status != ERR_RX_TIMEOUT:
                            print(f"[RX Error] {radio.get_status_text()}")
            except KeyboardInterrupt:
                print("\nReception stopped.")

if __name__ == "__main__":
    main()
