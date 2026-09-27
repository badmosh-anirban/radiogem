---
name: pyradiolib
description: >-
  Comprehensive guide for developing, building, debugging, extending, and using pyradiolib —
  a Python binding layer that exposes RadioLib's C++ LoRa/LoRaWAN radio implementations on
  Raspberry Pi and embedded Linux using lgpio and pybind11.
---

# `pyradiolib`: AI Agent Engineering Guide

This skill file teaches an AI coding agent how to understand, build, debug, use, and extend **`pyradiolib`**.

`pyradiolib` provides high-performance Python bindings for **RadioLib** (a widely adopted, battle-tested C++ RF communication library) targeting **Raspberry Pi and embedded Linux** single-board computers (SBCs).

---

## 1. Core Architectural Philosophy: Binding vs. Porting

> **CRITICAL RULE FOR AI AGENTS:**
> **DO NOT REWRITE OR REIMPLEMENT RADIOLIB'S RADIO PROTOCOLS IN PYTHON.**
>
> `pyradiolib` is **NOT** a Python rewrite of RadioLib. It is a thin, high-performance C++ binding layer using `pybind11` that wraps RadioLib's existing C++ implementation and official Raspberry Pi Hardware Abstraction Layer (`PiHal`).
>
> All RF timing, state machines, packet encoding, register maps, frequency synthesizers, and interrupt logic belong in **C++ RadioLib**. Python provides the user-facing application API.

### High-Level Architecture

```text
               ┌────────────────────────────────────────────────────────┐
               │              Python Application Layer                  │
               │   (user scripts, asyncio, telegram bots, gpiozero)     │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
               ┌────────────────────────────────────────────────────────┐
               │           pyradiolib (pybind11 bindings)               │
               │  - Exposes SX1262 class, Packet struct, status enums   │
               │  - Releases GIL on blocking operations (TX/RX)         │
               │  - Acquires GIL on interrupt callbacks                 │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
               ┌────────────────────────────────────────────────────────┐
               │          C++ Wrapper Layer (src/sx1262_wrapper.*)      │
               │  - Manages lifecycle of RadioLib SX1262 & Module       │
               │  - Bridges C++ callbacks to Python std::function       │
               │  - Handles Hardware CS (NSS) vs Software CS logic      │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
               ┌────────────────────────────────────────────────────────┐
               │                   RadioLib C++ Core                    │
               │  - SX126x driver, LoRa modulation, registers, packets  │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
               ┌────────────────────────────────────────────────────────┐
               │             PiHal (hal/PiHal.h - RadioLibHal)          │
               │  - RadioLib Hardware Abstraction Layer for Linux       │
               │  - Implements delay(), millis(), pinMode(), etc.       │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
               ┌────────────────────────────────────────────────────────┐
               │                      Linux lgpio                       │
               │  - lgSpiOpen() / lgSpiXfer() (/dev/spidevX.Y)         │
               │  - lgGpioClaimInput() / lgGpioClaimAlert()             │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
               ┌────────────────────────────────────────────────────────┐
               │         Hardware: SX1262 LoRa Transceiver              │
               │  (SPI: MOSI, MISO, SCLK, NSS | GPIO: RESET, BUSY, DIO1)│
               └────────────────────────────────────────────────────────┘
```

---

## 2. Directory Layout & Key Files

```text
radiogem/ (or pyradiolib root)
├── CMakeLists.txt              # Root build configuration for native_test & pyradiolib (.so)
├── pyproject.toml              # PEP 517/621 build spec (declares pybind11, cmake, setuptools)
├── setup.py                    # Custom setuptools CMakeExtension builder
├── MANIFEST.in                 # SDist packager manifest (bundles C++ headers & RadioLib/src)
├── PUBLISHING_GUIDE.md         # Guide for PyPI compilation and release
├── LICENSE                     # MIT License
├── RadioLib/                   # Vendored upstream RadioLib C++ source
│   ├── CMakeLists.txt          # RadioLib static library build target
│   └── src/                    # RadioLib.h, Module.h, modules/SX126x, etc.
├── hal/
│   └── PiHal.h                 # RadioLibHal implementation wrapping Linux lgpio
├── src/
│   ├── bindings.cpp            # Pybind11 module declaration, enums, Packet class, SX1262 methods
│   ├── sx1262_wrapper.h        # C++ wrapper header, Packet struct, method declarations
│   └── sx1262_wrapper.cpp      # C++ wrapper implementation, interrupt dispatchers
└── examples/
    ├── basic_tx.py             # Simple blocking transmit example
    ├── basic_rx.py             # Simple blocking receive example
    ├── interrupt_rx.py         # Asynchronous non-blocking interrupt RX example
    ├── change_settings.py      # Dynamic RF configuration (frequency, SF, power, BW)
    ├── gpiozero_coexist.py     # Safe coexistence pattern with gpiozero
    └── native_test.cpp         # Pure C++ standalone diagnostic tool (Stage 1 test)
```

---

## 3. Critical Hardware Realities & Edge Cases

When debugging or writing code for `pyradiolib`, keep these four hardware/OS quirks in mind:

### 1. The Kernel SPI Chip Select (CS / NSS) Trap
* **The Problem:** On Raspberry Pi, `/dev/spidev0.0` uses GPIO 8 (CE0), and `/dev/spidev0.1` uses GPIO 7 (CE1). When Linux `spidev` transfers SPI data via `lgSpiXfer()`, the Linux kernel automatically asserts and deasserts the hardware Chip Select line.
* If user code or RadioLib simultaneously attempts `lgGpioClaimOutput(7)` or `lgGpioWrite(7)`, the Linux kernel returns `EBUSY` or fights the hardware CS state.
* **The Solution in `sx1262_wrapper.cpp`:**
  ```cpp
  bool is_hw_cs = false;
  if (spi_device == 0) {
      if ((spi_channel == 0 && nss_pin == 8) || (spi_channel == 1 && nss_pin == 7)) {
          is_hw_cs = true;
      }
  }
  uint32_t mod_nss = is_hw_cs ? RADIOLIB_NC : nss_pin;
  mod = new Module(hal, mod_nss, dio1_pin, rst_pin, busy_pin);
  ```
  Passing `RADIOLIB_NC` tells RadioLib not to manually toggle the CS pin because the kernel driver handles it during SPI transactions.

### 2. The SX126x BUSY Pin Protocol
* The SX1262 has an active-high `BUSY` line.
* Before transmitting any command or register write over SPI, the host MUST wait for `BUSY` to go LOW.
* If `BUSY` is wired to the wrong GPIO or floating, every SPI transaction times out, returning `ERR_SPI_CMD_TIMEOUT (-706)` or `ERR_CHIP_NOT_FOUND (-2)`.

### 3. GPIO Coexistence with `gpiozero`
* `gpiozero` uses `lgpio` as its default backend on modern Raspberry Pi OS (Bookworm).
* `pyradiolib` also uses `lgpio` directly in C++.
* **Rule:** Two separate libraries cannot claim the *same* physical GPIO pin simultaneously.
* **Safe Pattern:** Dedicate specific pins to the LoRa module (e.g., NSS=7, DIO1=17, RST=22, BUSY=24) and dedicate other pins to `gpiozero` (e.g., LED=21, Button=20). They will coexist in the same process seamlessly.

### 4. Releasing and Acquiring the Python GIL
* Blocking operations (`transmit()`, `receive()`) release the GIL via `py::call_guard<py::gil_scoped_release>()`. This ensures background Python threads, asyncio loops, or web servers remain responsive while the radio is transmitting or awaiting a packet.
* When hardware interrupts fire on DIO1, the C++ alert handler must acquire the GIL using `py::gil_scoped_acquire` before invoking any Python callback function.

---

## 4. Systematic 3-Stage Debugging Methodology

When troubleshooting an issue (e.g., "radio won't start", "receive hangs", or "packet not transmitted"), **ALWAYS isolate the problem in three distinct stages**:

```text
[Stage 1: Native C++ Verification] ──► Confirms Hardware Wiring, Kernel SPI, and lgpio
            │
            ▼
[Stage 2: Python Binding Verification] ─► Confirms Pybind11 Module & Status Codes
            │
            ▼
[Stage 3: Application & RF Verification] ─► Confirms Frequency, LoRa Sync Word, Antennas
```

### Stage 1: Native C++ Diagnostic
Before debugging Python, rule out hardware wiring, Linux device permissions, or kernel SPI issues:
```bash
# Build the native C++ test binary
mkdir -p build && cd build
cmake .. -DBUILD_NATIVE_TEST=ON
make native_test -j$(nproc)
./native_test
```
* If `native_test` fails to find the chip (`ERR_CHIP_NOT_FOUND`), check:
  1. Is SPI enabled? (`ls -l /dev/spidev*` must exist; enable via `raspi-config`).
  2. Is `liblgpio-dev` installed? (`sudo apt install -y liblgpio-dev`).
  3. Are 3.3V, GND, MOSI (GPIO 10), MISO (GPIO 9), SCLK (GPIO 11), NSS, RESET, and BUSY firmly connected?

### Stage 2: Python Binding Test
Test the compiled module directly in Python:
```python
import pyradiolib
print("pyradiolib status:", pyradiolib.ERR_NONE)
radio = pyradiolib.SX1262(spi_channel=1, nss=7, dio1=17, reset=22, busy=24)
status = radio.begin(frequency=866.5, bandwidth=125.0, spreading_factor=7, coding_rate=5)
print(f"begin() status: {status} ({pyradiolib.get_status_text(status)})")
assert status == pyradiolib.ERR_NONE, "SX1262 initialization failed!"
```

### Stage 3: RF Link & Packet Verification
If initialization succeeds but packets are not received:
* **LoRa Sync Word:** Both transmitter and receiver must match!
  * Private network: `sync_word = 0x12` (`RADIOLIB_SX126X_SYNC_WORD_PRIVATE`)
  * Public network / LoRaWAN: `sync_word = 0x34`
* **TCXO Voltage:** Some SX1262 breakout boards have a Temperature-Compensated Crystal Oscillator (TCXO) powered via DIO3.
  * If the board uses a TCXO (common on Waveshare modules), set `tcxo_voltage=1.6` (or `1.8`, `2.4`, `3.3`).
  * If the board uses an external quartz crystal (XTAL), set `tcxo_voltage=0.0`.
* **Antenna:** Never transmit without an antenna attached (can damage the power amplifier).

---

## 5. RadioLib Error Codes Lookup Table

All RadioLib status codes are exposed directly as integer constants on the `pyradiolib` module:

| Constant | Value | Meaning | Primary Causes & Fixes |
| :--- | :--- | :--- | :--- |
| `ERR_NONE` | `0` | Success | Operation completed successfully. |
| `ERR_CHIP_NOT_FOUND` | `-2` | Chip ID mismatch | SPI wiring incorrect, wrong NSS/BUSY pin, SPI interface not enabled in `/boot/config.txt`. |
| `ERR_PACKET_TOO_LONG`| `-4` | Payload > 256 bytes | LoRa packet payload exceeds SX1262 hardware FIFO buffer. |
| `ERR_TX_TIMEOUT` | `-5` | Transmission timed out | Radio failed to emit packet, RF switch control pin failed, or power amplifier fault. |
| `ERR_RX_TIMEOUT` | `-6` | Receive timed out | No packet arrived within specified `timeout_ms`. Normal if channel is idle. |
| `ERR_CRC_MISMATCH` | `-7` | Corrupted packet | Signal too weak, noise on frequency, or mismatch in CRC settings. |
| `ERR_INVALID_BANDWIDTH` | `-8` | Invalid BW | SX1262 only supports specific bandwidths (e.g. 7.8, 10.4, ..., 125.0, 250.0, 500.0 kHz). |
| `ERR_INVALID_SPREADING_FACTOR` | `-9` | Invalid SF | Valid SF range is 5 to 12. |
| `ERR_INVALID_CODING_RATE` | `-10` | Invalid CR | Valid coding rate denominators are 5 to 8 (for 4/5 through 4/8). |
| `ERR_INVALID_OUTPUT_POWER` | `-13` | Invalid Power | SX1262 supports power levels from -9 dBm up to +22 dBm. |
| `ERR_SPI_CMD_TIMEOUT`| `-706` | BUSY pin hung | SX1262 `BUSY` pin was not de-asserted within timeout. Check BUSY pin wiring. |
| `ERR_SPI_CMD_INVALID`| `-707` | Invalid SPI opcode | Opcode rejected by radio modem firmware. |
| `ERR_SPI_CMD_FAILED` | `-708` | Command execution error| Radio modem rejected parameter or is in an invalid state. |

---

## 6. How to Build and Compile `pyradiolib`

### System Requirements (Raspberry Pi / Linux)
```bash
sudo apt update
sudo apt install -y build-essential cmake python3-dev liblgpio-dev python3-lgpio
```

### A. Direct Development Build (CMake)
```bash
mkdir -p build && cd build
cmake .. -DPYTHON_EXECUTABLE=$(which python3) -DCMAKE_BUILD_TYPE=Release
make pyradiolib -j$(nproc)
# Creates pyradiolib.cpython-*.so in build/
```

### B. Python Package Local Install (`pip`)
```bash
# Inside project root
pip install .

# For editable/development mode:
pip install -e . --no-build-isolation
```

### C. Building PyPI Distribution Packages (sdist & wheel)
```bash
pip install --upgrade build twine
rm -rf build/ dist/ *.egg-info
python -m build
# Generates dist/pyradiolib-0.1.0.tar.gz and dist/pyradiolib-*-linux_*.whl
twine check dist/*
```

---

## 7. Python API Reference & Idiomatic Patterns

### 1. Initialization and Context Management
```python
import pyradiolib

# Always use context manager or explicit close() to ensure GPIOs are freed on exit:
with pyradiolib.SX1262(
    spi_channel=1,      # SPI0 CE1 (/dev/spidev0.1)
    spi_speed=2000000,  # 2 MHz
    spi_device=0,       # SPI bus 0
    gpio_device=0,      # /dev/gpiochip0
    nss=7,              # GPIO 7
    dio1=17,            # GPIO 17 (Interrupt)
    reset=22,           # GPIO 22
    busy=24             # GPIO 24
) as radio:
    status = radio.begin(
        frequency=866.5,          # MHz
        bandwidth=125.0,          # kHz
        spreading_factor=7,       # SF7
        coding_rate=5,            # 4/5
        sync_word=0x12,           # Private LoRa
        power=10,                 # dBm
        preamble_length=8,        # Symbols
        tcxo_voltage=1.6,         # 1.6V TCXO (0.0 for crystal)
        use_regulator_ldo=False   # Internal DC-DC converter
    )
    if status != pyradiolib.ERR_NONE:
        raise RuntimeError(f"Radio init failed: {pyradiolib.get_status_text(status)}")
```

### 2. Blocking Transmit and Receive
```python
# Transmit string or bytes (releases GIL during transmission)
tx_status = radio.transmit("Hello from Raspberry Pi")
if tx_status == pyradiolib.ERR_NONE:
    print("Transmit success!")

# Receive with timeout in milliseconds (releases GIL while awaiting packet)
packet = radio.receive(timeout_ms=5000)
if packet is not None:
    print(f"Received {len(packet)} bytes:")
    print(f"  Text: {packet.text}")
    print(f"  Raw:  {packet.payload}")
    print(f"  RSSI: {packet.rssi:.1f} dBm")
    print(f"  SNR:  {packet.snr:.1f} dB")
else:
    print("RX timeout or error")
```

### 3. Asynchronous / Interrupt-Driven RX
```python
import time

def on_packet():
    packet = radio.read_data()
    if packet:
        print(f"[Async RX] Got: {packet.text} (RSSI: {packet.rssi} dBm)")
    # Re-enable listening for the next packet
    radio.start_receive(timeout_ms=0)

# Register Python callback
radio.set_packet_received_action(on_packet)

# Put radio in continuous listening mode
radio.start_receive(timeout_ms=0)

try:
    while True:
        time.sleep(1)
finally:
    radio.clear_packet_received_action()
    radio.standby()
```

---

## 8. Step-by-Step Guide to Extending `pyradiolib`

When asked to add a new feature (e.g., FSK modulation, SX1268, CAD, or packet RSSI polling):

### Workflow for Adding a Feature
1. **Locate the upstream RadioLib C++ API**:
   Look inside `RadioLib/src/modules/SX126x/SX1262.h` or `RadioLib/src/TypeDef.h` to see how RadioLib implements the function natively.
2. **Update the C++ Wrapper Header (`src/sx1262_wrapper.h`)**:
   Add the public method to `SX1262Wrapper`:
   ```cpp
   int16_t scan_channel();
   ```
3. **Implement the Method in `src/sx1262_wrapper.cpp`**:
   Delegate cleanly to the RadioLib pointer:
   ```cpp
   int16_t SX1262Wrapper::scan_channel() {
       if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
       last_status = radio->scanChannel();
       return last_status;
   }
   ```
4. **Expose the Method in `src/bindings.cpp`**:
   Register it with pybind11 with clear docstrings and argument tags:
   ```cpp
   .def("scan_channel", &SX1262Wrapper::scan_channel,
        "Perform LoRa Channel Activity Detection (CAD). Returns status code (0 = channel clear, RADIOLIB_CHANNEL_FREE)")
   ```
5. **Add a Python Example / Unit Test in `examples/`**:
   Create a test script validating the new functionality.

---

## 9. Agent Checklist: Common Anti-Patterns to Avoid

- [ ] **NEVER duplicate protocol logic:** Do not write bit-packing, Hamming coding, CRC checks, or SPI byte sequences in Python. RadioLib already does this in C++.
- [ ] **NEVER leave GPIO pins open on crash:** Always expose and use `close()` or `__exit__()` to invoke `hal->term()` and close file descriptors in `lgpio`.
- [ ] **NEVER hard-code hardware pin numbers:** Always provide constructor arguments with sensible defaults (`spi_channel=1`, `nss=7`, `dio1=17`, `reset=22`, `busy=24`).
- [ ] **NEVER break `MANIFEST.in`:** If adding new C++ headers or files, ensure `MANIFEST.in` includes them so `pip install` from source distribution packages doesn't break on user machines.
- [ ] **NEVER mix GPIO pins with other libraries:** Ensure `gpiozero` and `pyradiolib` use strictly disjoint sets of GPIO numbers.
