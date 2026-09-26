# Project Context: Building a Python RadioLib Module for Raspberry Pi + SX1262

## 1. Objective

I want to create a Python library/module called **`pyradiolib`** that allows a Raspberry Pi to use the **RadioLib C++ library**, particularly its **SX1262 support**, from Python.

The ultimate goal is NOT to rewrite the entire RadioLib C++ codebase in Python.

Instead, I want to:

1. Reuse the existing RadioLib C++ implementation.
2. Reuse RadioLib's official Raspberry Pi `PiHal.h`.
3. Reuse the Linux `lgpio` GPIO/SPI functionality already implemented by `PiHal`.
4. Create a thin C++/Python binding layer, preferably using **pybind11**.
5. Expose a clean, Python-friendly API for SX1262.
6. Build the resulting Python extension directly on Raspberry Pi.
7. Make it possible to use `pyradiolib` alongside Python libraries such as `gpiozero`.

The desired architecture is:

```text
Python application
        │
        ▼
   pyradiolib
        │
   pybind11 bindings
        │
        ▼
 Existing RadioLib C++
        │
        ▼
      PiHal
        │
        ▼
      lgpio
        │
        ├── Linux SPI
        └── Linux GPIO
        │
        ▼
      SX1262
```

The key principle is:

> Do not port/reimplement RadioLib's radio protocol implementation in Python. Wrap the existing C++ implementation.

---

# 2. Why this is possible

RadioLib is primarily known for Arduino/ESP32/etc., but the repository already provides a **non-Arduino Raspberry Pi example**.

The supplied official example is explicitly titled:

> RadioLib Non-Arduino Raspberry Pi Example

It states that the example demonstrates how to use RadioLib without Arduino, using a Raspberry Pi and the `lgpio` library.

It also states that the example can be used as a starting point for porting RadioLib to another platform and points to RadioLib's HAL API documentation.

The example includes:

```cpp
#include <RadioLib.h>
#include "hal/RPi/PiHal.h";
```

It then creates:

```cpp
PiHal* hal = new PiHal(1);
```

and creates the radio:

```cpp
SX1261 radio = new Module(
    hal,
    7,
    17,
    22,
    RADIOLIB_NC
);
```

The radio is then initialized with:

```cpp
int state = radio.begin({});
```

and used normally:

```cpp
state = radio.transmit(str);
```

Therefore, the important discovery is:

> RadioLib itself already has a Raspberry Pi/Linux hardware abstraction layer. We do not need to invent the Pi hardware interface.

---

# 3. Official Raspberry Pi HAL

The supplied `PiHal.h` is an official RadioLib Raspberry Pi HAL using `lgpio`.

The beginning of the file establishes:

```cpp
#include <RadioLib.h>
#include <lgpio.h>
```

The class is:

```cpp
class PiHal : public RadioLibHal
```

This is extremely important.

RadioLib defines a generic `RadioLibHal` interface. `PiHal` implements that interface for Raspberry Pi/Linux.

The constructor is:

```cpp
PiHal(
    uint8_t spiChannel,
    uint32_t spiSpeed = 2000000,
    uint8_t spiDevice = 0,
    uint8_t gpioDevice = 0
)
```

and initializes the base HAL with Raspberry Pi GPIO/input/output definitions.

Therefore:

```text
RadioLib generic HAL
        ▲
        │
        │ implements
        │
      PiHal
        │
        ▼
      lgpio
```

This is the foundation of the project.

---

# 4. What PiHal actually provides

The supplied `PiHal.h` implements the functions RadioLib requires from a hardware abstraction layer.

## GPIO

`pinMode()` uses:

```cpp
lgGpioClaimInput()
lgGpioClaimOutput()
```

`digitalWrite()` uses:

```cpp
lgGpioWrite()
```

`digitalRead()` uses:

```cpp
lgGpioRead()
```

Therefore RadioLib's generic GPIO operations are translated into Linux `lgpio` calls.

---

# 5. Interrupt handling

RadioLib requires interrupt functionality, particularly for LoRa modules such as the SX126x family.

`PiHal` implements:

```cpp
attachInterrupt()
detachInterrupt()
```

using `lgpio` alert functionality.

Specifically, it uses:

```cpp
lgGpioClaimAlert()
```

and:

```cpp
lgGpioSetAlertsFunc()
```

The HAL maintains arrays for:

```cpp
interruptEnabled[]
interruptModes[]
interruptCallbacks[]
```

and an `lgpioAlertHandler()` translates the `lgpio` alert into the callback expected by RadioLib.

Therefore the Raspberry Pi HAL is not merely doing SPI. It also provides the GPIO interrupt mechanism RadioLib expects.

---

# 6. Timing functions

The HAL implements:

```cpp
delay()
delayMicroseconds()
yield()
millis()
micros()
```

using Linux/`lgpio` timing facilities.

For example:

```cpp
lguSleep()
```

is used for delays.

`lguTimestamp()` is used to implement millisecond/microsecond timing.

This means RadioLib code that expects Arduino-like timing functions can continue to operate through the HAL without Arduino itself being installed.

---

# 7. SPI implementation

This is another critical part.

`PiHal::spiBegin()` uses:

```cpp
lgSpiOpen()
```

and `spiTransfer()` uses:

```cpp
lgSpiXfer()
```

Specifically:

```cpp
int result = lgSpiXfer(
    _spiHandle,
    (char *)out,
    (char*)in,
    len
);
```

Therefore the SPI chain is:

```text
RadioLib
   │
   ▼
PiHal::spiTransfer()
   │
   ▼
lgSpiXfer()
   │
   ▼
Linux SPI
   │
   ▼
SX1262
```

This is exactly what we want to preserve.

---

# 8. PiHal configuration

The HAL stores:

```cpp
_gpioDevice
_spiDevice
_spiSpeed
_spiChannel
```

and creates the GPIO and SPI handles when initialized.

The constructor defaults include:

```text
SPI speed: 2 MHz
SPI device: 0
GPIO device: 0
```

with the SPI channel supplied by the caller.

The official example uses:

```cpp
PiHal* hal = new PiHal(1);
```

meaning SPI channel 1.

However, the Python library should NOT hard-code these values. They should be configurable.

The Python API should eventually allow something like:

```python
radio = SX1262(
    spi_channel=1,
    spi_device=0,
    spi_speed=2000000,
    gpio_device=0,
    nss=7,
    dio1=17,
    reset=22,
    busy=24
)
```

---

# 9. SX1262 instead of SX1261

The official Raspberry Pi example supplied with the RadioLib source is an **SX1261** example.

The example contains:

```cpp
SX1261 radio = new Module(
    hal,
    7,
    17,
    22,
    RADIOLIB_NC
);
```

The project itself is named:

```text
rpi-sx1261
```

However, the purpose of `PiHal` is hardware abstraction, not SX1261-specific radio functionality.

The target project here is specifically:

> Raspberry Pi + SX1262

Therefore the wrapper should instantiate RadioLib's **SX1262** class instead of SX1261.

The exact SX1262 constructor/API must be checked against the specific RadioLib version being used before final implementation.

Do not assume that every RadioLib version has exactly the same constructor signatures.

---

# 10. The existing official build system

The supplied official Raspberry Pi CMake configuration is very simple.

It does:

```cmake
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../RadioLib"
    "${CMAKE_CURRENT_BINARY_DIR}/RadioLib"
)
```

Then:

```cmake
add_executable(
    ${PROJECT_NAME}
    main.cpp
)
```

and links:

```cmake
target_link_libraries(
    ${PROJECT_NAME}
    RadioLib
    lgpio
)
```

This demonstrates that the native Raspberry Pi application is simply:

```text
Application
   │
   ├── RadioLib
   │
   └── lgpio
```

We want to extend this architecture to:

```text
Python
   │
   ▼
pybind11 extension
   │
   ├── RadioLib
   │
   └── lgpio
```

---

# 11. Proposed Python architecture

The recommended architecture is:

```text
                Python
                  │
                  ▼
           pyradiolib module
                  │
                  ▼
             pybind11
                  │
                  ▼
          C++ wrapper class
                  │
                  ▼
             RadioLib
                  │
                  ▼
               PiHal
                  │
                  ▼
               lgpio
                  │
          ┌───────┴────────┐
          ▼                ▼
        GPIO              SPI
          │                │
          └───────┬────────┘
                  ▼
                SX1262
```

Python should never directly manipulate RadioLib internals.

---

# 12. Why pybind11

The preferred binding technology is **pybind11**.

The reason is that RadioLib is already C++, and pybind11 allows us to expose selected C++ classes/functions to Python without rewriting the underlying library.

For example, C++ could internally have:

```cpp
class SX1262Wrapper {
public:
    int begin(...);
    int transmit(...);
    std::string receive(...);
};
```

and pybind11 could expose:

```python
radio.begin(...)
radio.transmit(...)
radio.receive(...)
```

Python would therefore see a normal Python module:

```python
import pyradiolib
```

while the actual radio implementation remains C++.

---

# 13. Do NOT wrap the entire RadioLib API initially

RadioLib is a large library.

The first version should be intentionally minimal.

Expose only what is necessary for an SX1262 application.

Initial API:

```python
radio = SX1262(...)

radio.begin(...)

radio.transmit(...)

radio.receive(...)

radio.rssi()

radio.snr()
```

Potential later APIs:

```python
radio.sleep()
radio.standby()
radio.set_frequency()
radio.set_bandwidth()
radio.set_spreading_factor()
radio.set_coding_rate()
radio.set_output_power()
```

Do not expose hundreds of RadioLib methods until there is an actual requirement.

---

# 14. Desired Python API

The ideal initial usage should look approximately like:

```python
from pyradiolib import SX1262

radio = SX1262(
    spi_channel=1,
    spi_device=0,
    spi_speed=2000000,

    nss=7,
    dio1=17,
    reset=22,
    busy=24
)

state = radio.begin(
    frequency=866.5,
    bandwidth=125.0,
    spreading_factor=7,
    coding_rate=5,
    power=10
)

print("Initialization:", state)

state = radio.transmit(
    "Hello from Raspberry Pi"
)

print("TX:", state)

packet = radio.receive(
    timeout_ms=5000
)

print(packet)
```

Eventually, `receive()` should ideally return a Python object containing:

```text
payload
RSSI
SNR
```

For example:

```python
packet.payload
packet.rssi
packet.snr
```

rather than requiring the user to call separate functions.

---

# 15. gpiozero compatibility

The Raspberry Pi application may also use Python's `gpiozero`.

`gpiozero` is a Python abstraction over GPIO backends such as `lgpio`.

The desired architecture is:

```text
Python
│
├── gpiozero
│      │
│      └── GPIO devices
│
└── pyradiolib
       │
       └── PiHal
              │
              └── lgpio
```

This is compatible as long as the same GPIO line is not simultaneously claimed/controlled by both systems.

For example:

```text
GPIO21 → gpiozero LED
GPIO20 → gpiozero Button

GPIO7  → SX1262 NSS
GPIO17 → SX1262 DIO1
GPIO22 → SX1262 RESET
GPIO24 → SX1262 BUSY
```

This is preferable to modifying `PiHal` to call gpiozero.

RadioLib should continue to use its official C++ `PiHal` and `lgpio` implementation.

`gpiozero` should remain a separate Python-level GPIO interface for unrelated application hardware.

---

# 16. Important GPIO ownership consideration

The agent must carefully consider GPIO ownership.

Do NOT design the system so that:

```text
gpiozero
     │
     ▼
GPIO17
     ▲
     │
PiHal
```

if both libraries attempt to claim/control GPIO17.

The SX1262-specific pins should be owned by `PiHal`.

Other application GPIOs can be managed by `gpiozero`.

---

# 17. Recommended project structure

The initial project should look approximately like:

```text
pyradiolib/
│
├── CMakeLists.txt
├── pyproject.toml
├── README.md
│
├── src/
│   ├── bindings.cpp
│   ├── sx1262_wrapper.cpp
│   └── sx1262_wrapper.h
│
├── hal/
│   └── PiHal.h
│
├── RadioLib/
│   └── ...
│
└── examples/
    └── basic_tx_rx.py
```

Ideally, RadioLib should be included as a Git submodule rather than manually copying source files.

---

# 18. C++ wrapper responsibilities

The wrapper should:

1. Construct `PiHal`.
2. Construct the RadioLib SX1262 object.
3. Initialize the radio.
4. Expose transmit.
5. Expose receive.
6. Expose RSSI/SNR.
7. Handle C++/RadioLib errors appropriately.
8. Cleanly destroy the radio and HAL.
9. Avoid exposing unnecessary RadioLib internals to Python.

The wrapper should NOT duplicate:

* SX1262 packet handling
* LoRa modulation logic
* SPI protocol
* register handling
* interrupt implementation
* GPIO implementation

Those already belong to RadioLib/PiHal.

---

# 19. Python binding responsibilities

`bindings.cpp` should only translate the wrapper into a Python API.

For example:

```cpp
PYBIND11_MODULE(pyradiolib, m) {
    py::class_<SX1262Wrapper>(m, "SX1262")
        .def(...)
        .def("begin", ...)
        .def("transmit", ...)
        .def("receive", ...);
}
```

Keep this layer thin.

---

# 20. Build system

The project should use CMake because the official Raspberry Pi RadioLib example already uses CMake.

The existing official build script essentially does:

```bash
mkdir -p build
cd build
cmake ..
make
```

Therefore the new project should preserve the same native build model while adding pybind11.

Expected dependencies:

```text
C++ compiler
CMake
Python development headers
pybind11
lgpio development library
RadioLib
```

---

# 21. Important distinction: this is a binding, not a port

The project should be described accurately as:

> A Python binding/wrapper around RadioLib's C++ SX1262 implementation using RadioLib's official Raspberry Pi `PiHal`.

It should NOT be described as:

> A complete Python rewrite/port of RadioLib.

The underlying implementation remains:

```text
C++ RadioLib
```

The Python layer is an interface to it.

---

# 22. Why this approach is preferable

A complete Python rewrite would require reproducing:

* SX126x register operations
* LoRa packet handling
* modulation configuration
* interrupts
* SPI communication
* timing
* error handling
* RadioLib abstractions
* radio state management

That would be unnecessary and would potentially diverge from RadioLib.

The binding approach allows us to reuse the existing tested implementation.

---

# 23. Important technical issue: Python and asynchronous radio operation

The SX1262 uses DIO1 for events such as transmission completion and packet reception.

The official `PiHal` already implements interrupt/alert handling through `lgpio`.

The Python wrapper should therefore NOT replace the RadioLib interrupt mechanism with Python polling unless there is a specific reason.

RadioLib should continue using:

```text
SX1262 DIO1
     │
     ▼
PiHal interrupt handling
     │
     ▼
lgpio alert callback
     │
     ▼
RadioLib callback/state
```

Python should interact with the higher-level radio API.

If blocking `receive()` is used initially, that is acceptable for the first implementation.

Later, asynchronous Python APIs can be considered.

---

# 24. Potential Python threading/GIL issue

If pybind11 is used, consider whether long-running RadioLib calls should release the Python GIL.

For example, if:

```python
radio.receive(timeout_ms=10000)
```

blocks for 10 seconds, the wrapper may eventually use pybind11's GIL-release mechanism around the blocking C++ call.

However, this should only be added after the basic implementation works correctly.

Do not introduce unnecessary concurrency complexity into the first prototype.

---

# 25. Error handling

RadioLib returns integer status codes such as:

```cpp
RADIOLIB_ERR_NONE
```

The first version may simply expose the integer:

```python
state = radio.begin(...)
```

Later, the Python wrapper could translate RadioLib errors into Python exceptions.

For example:

```python
try:
    radio.begin(...)
except RadioError as e:
    print(e)
```

But this is a second-stage improvement.

Initially, preserving RadioLib's status codes makes debugging easier.

---

# 26. Hardware target

The immediate target is:

```text
Raspberry Pi
    +
SX1262 module
```

The SX1262 should be connected using:

```text
SPI
NSS/CS
DIO1
RESET
BUSY
```

The exact GPIO numbers must be configurable.

Do not hard-code the GPIO numbers from the official SX1261 example because that example is for a specific Waveshare board.

For example, its pinout contains:

```text
NSS  = 7
DIO1 = 17
NRST = 22
BUSY = not connected
```

Those values belong to the example hardware configuration and should not automatically be assumed for another SX1262 module.

---

# 27. Radio settings for my current project

My current LoRa prototype uses approximately:

```text
Frequency:           866.5 MHz
Bandwidth:           125 kHz
Spreading Factor:   7
Coding Rate:        5
Output Power:       10 dBm
```

The exact RadioLib API should be checked against the current RadioLib version before implementing `begin()`.

The library should not hard-code these values; Python should be able to configure them.

---

# 28. Current development strategy

Build the project in stages.

## Stage 1 — Native C++ verification

Before introducing Python, verify:

```text
RadioLib
+
official PiHal
+
SX1262
+
lgpio
```

works natively on Raspberry Pi.

Use the official non-Arduino example as the reference.

---

## Stage 2 — Minimal C++ wrapper

Create:

```text
SX1262Wrapper
```

with:

```text
constructor
begin()
transmit()
receive()
RSSI
SNR
destructor
```

---

## Stage 3 — pybind11

Expose the wrapper as:

```python
import pyradiolib
```

---

## Stage 4 — Python TX test

Create:

```python
radio.begin(...)
radio.transmit("Hello")
```

and verify another LoRa device receives it.

---

## Stage 5 — Python RX test

Test:

```python
packet = radio.receive(...)
```

and verify:

```text
payload
RSSI
SNR
```

---

## Stage 6 — gpiozero coexistence

Add unrelated GPIO devices through:

```python
from gpiozero import LED, Button
```

while keeping SX1262 GPIOs under `PiHal`.

---

## Stage 7 — Production-quality Python API

Eventually add:

* Python exceptions
* packet object
* configuration object
* asynchronous receive
* callbacks
* context manager
* clean shutdown
* packaging/install support
* documentation

---

# 29. What I want the AI coding agent to do

The coding agent should first inspect:

1. The exact RadioLib version.
2. The exact `PiHal.h`.
3. The exact SX1262 API in that RadioLib version.
4. The official Raspberry Pi example.
5. The existing CMake configuration.

Do not assume APIs from a different RadioLib release.

Then design the smallest possible working `pyradiolib` implementation.

The first milestone should be:

```python
from pyradiolib import SX1262

radio = SX1262(...)

status = radio.begin(...)

print(status)

status = radio.transmit("Hello")

print(status)
```

Once this works reliably, add receive support.

---

# 30. Critical constraints

The implementation should follow these principles:

### Do

* Reuse official RadioLib.
* Reuse official `PiHal`.
* Use `lgpio` through `PiHal`.
* Use pybind11 for Python bindings.
* Keep the Python API small initially.
* Make GPIO/SPI configuration configurable.
* Match the exact RadioLib version.
* Test native C++ first.
* Test TX before RX.
* Keep gpiozero separate from SX1262 GPIO ownership.

### Do not

* Rewrite RadioLib in Python.
* Reimplement the SX1262 driver.
* Replace PiHal with gpiozero.
* Duplicate SPI handling in Python.
* Hard-code the official example's GPIO mapping for arbitrary SX1262 hardware.
* Assume the SX1261 example is automatically an SX1262 implementation.
* Expose the entire RadioLib API unnecessarily.
* Introduce asynchronous/threading complexity before basic TX/RX works.

---

# 31. Desired final result

The ultimate goal is a Raspberry Pi Python library that feels like a normal Python LoRa library:

```python
from pyradiolib import SX1262

radio = SX1262(
    spi_channel=1,
    spi_device=0,
    nss=7,
    dio1=17,
    reset=22,
    busy=24
)

radio.begin(
    frequency=866.5,
    bandwidth=125,
    spreading_factor=7,
    coding_rate=5,
    power=10
)

radio.transmit("Hello from Raspberry Pi")

packet = radio.receive(timeout_ms=5000)

if packet:
    print(packet.payload)
    print(packet.rssi)
    print(packet.snr)
```

But internally:

```text
Python API
    ↓
pybind11
    ↓
C++ wrapper
    ↓
RadioLib SX1262
    ↓
RadioLib Module
    ↓
PiHal
    ↓
lgpio
    ↓
Linux SPI/GPIO
    ↓
SX1262
```

The major objective is to make this possible **without modifying the core RadioLib SX1262 implementation**.

---

# 32. Source material supplied for this project

The supplied official Raspberry Pi example contains the key architecture:

```cpp
#include <RadioLib.h>
#include "hal/RPi/PiHal.h"

PiHal* hal = new PiHal(1);

SX1261 radio = new Module(
    hal,
    7,
    17,
    22,
    RADIOLIB_NC
);

radio.begin({});
radio.transmit(str);
```

The supplied CMake file builds the native application by adding RadioLib as a subdirectory and linking:

```cmake
target_link_libraries(${PROJECT_NAME} RadioLib lgpio)
```

The supplied build script performs:

```bash
mkdir -p build
cd build
cmake ...
make
```

These demonstrate that the Raspberry Pi/Linux path is already supported at the C++ HAL level.

The new work is therefore primarily the **Python binding layer**, not the Raspberry Pi hardware port itself.

---

# 33. Core question for the coding agent

The main engineering question is:

> What is the cleanest, smallest, and most maintainable way to expose RadioLib's existing SX1262 C++ implementation and official Raspberry Pi `PiHal` to Python using pybind11, while preserving RadioLib's GPIO/SPI/interrupt handling through `lgpio` and allowing unrelated GPIO devices to coexist through gpiozero?

Start by producing a **compileable minimal prototype**, not a complete production library.
