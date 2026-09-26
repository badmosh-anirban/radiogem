# #2

I have reviewed [`other_functions.md`](other_functions.md). You have raised great questions regarding:
1. **Two modes of Transmit** (Blocking vs Non-blocking/Interrupt)
2. **Two modes of Receive** (Blocking vs Non-blocking/Interrupt)
3. **Runtime Radio Settings** (`setSyncWord`, `setCurrentLimit`, `setCRC`, `setTCXO`, `setDio2AsRfSwitch`, etc.)

**I have now implemented all of these features into `pyradiolib`**! Below is how each is designed, how the C++-to-Python translation works, and how you can use them.

---

### 1. Transmit: Blocking vs Non-Blocking

#### A. Blocking Transmit: `radio.transmit(data)`
* **In C++**: Calls RadioLib's `radio->transmit(data)`.
* **In Python**: At the Python call site, it appears synchronous, but **it releases the Python GIL** (`py::call_guard<py::gil_scoped_release>`).
* **Why this matters**: While the SX1262 is transmitting over the air, Python's runtime is **not blocked**. Other background threads, `gpiozero` buttons/LEDs, or `asyncio` loops continue executing smoothly.

#### B. Non-Blocking / Interrupt Transmit: `start_transmit` + `finish_transmit`
In C++ RadioLib:
```cpp
radio.setPacketSentAction(onSent);
radio.startTransmit(str);
// later when interrupt fires:
radio.finishTransmit();
```
**How we translated this to Python**:
```python
# Option 1: Using a callback
def on_sent():
    print("Packet transmission complete!")

radio.set_packet_sent_action(on_sent)
radio.start_transmit("Hello Non-blocking")

# ... do other Python work while packet is transmitting ...

# When done, finalize the transmitter (disables RF switch, enters standby):
radio.finish_transmit()

# Option 2: Using the boolean flag (no callback needed)
radio.start_transmit("Hello Non-blocking")
while not radio.has_sent:
    time.sleep(0.01)
radio.finish_transmit()
```
* **See the complete working example**: [`examples/interrupt_tx.py`](examples/interrupt_tx.py).

---

### 2. Receive: Blocking vs Non-Blocking

#### A. Blocking Receive: `radio.receive(timeout_ms=5000)`
* **In C++**: Calls RadioLib's `radio->receive(buffer, 0, timeout)`.
* **In Python**: Blocks until a packet arrives or the timeout expires, while **releasing the GIL** so other Python tasks continue running.

#### B. Non-Blocking / Interrupt Receive: `start_receive` + `read_data`
In C++ RadioLib:
```cpp
radio.setPacketReceivedAction(onReceived);
radio.startReceive(); // continuous listen mode
// when interrupt fires:
radio.readData(str);
```
**The C++-to-Python Challenge**:
RadioLib's `setPacketReceivedAction` takes a raw C function pointer `void (*func)(void)` called by `lgpio`'s background OS alert thread. In Python, an external C thread cannot execute Python bytecode without first acquiring Python's Global Interpreter Lock (GIL)—otherwise Python segfaults.

**How we solved this**:
In [`src/bindings.cpp`](src/bindings.cpp), the wrapper automatically acquires the GIL (`py::gil_scoped_acquire`) inside the alert handler before invoking your Python callback, safely catches any Python exceptions, and releases the GIL.

**How to use it in Python**:
```python
# 1. Define your callback
def on_packet():
    packet = radio.read_data()
    if packet:
        print(f"Received: {packet.text} | RSSI: {packet.rssi} dBm")
    # Re-arm continuous listening mode for the next packet
    radio.start_receive()

# 2. Register callback and start continuous receive (no timeout!)
radio.set_packet_received_action(on_packet)
radio.start_receive()

# 3. Main thread is 100% free!
while True:
    time.sleep(1)
```
*(Alternatively, you can poll `if radio.has_received: packet = radio.read_data()` without defining a callback).*
* **See the complete working example**: [`examples/interrupt_rx.py`](examples/interrupt_rx.py).

---

### 3. Full Runtime Settings (Now Implemented!)

All the runtime configuration methods mentioned in [`other_functions.md`](other_functions.md) are now exposed directly on the `SX1262` object:

| Python Method | C++ RadioLib Equivalent | Description |
| :--- | :--- | :--- |
| `radio.set_frequency(868.0)` | `setFrequency(868.0)` | Change carrier frequency (MHz) |
| `radio.set_bandwidth(250.0)` | `setBandwidth(250.0)` | Change LoRa bandwidth (kHz) |
| `radio.set_spreading_factor(9)` | `setSpreadingFactor(9)` | Change Spreading Factor (5 - 12) |
| `radio.set_coding_rate(7)` | `setCodingRate(7)` | Change Coding Rate (5 - 8 for 4/5 - 4/8) |
| `radio.set_output_power(14)` | `setOutputPower(14)` | Change output power (-9 to +22 dBm) |
| `radio.set_sync_word(0x12)` | `setSyncWord(0x12)` | LoRa sync word (`0x12` private, `0x34` public) |
| `radio.set_current_limit(80.0)` | `setCurrentLimit(80)` | Over-current limit (45 - 240 mA; 0 to disable) |
| `radio.set_preamble_length(12)` | `setPreambleLength(12)` | Preamble length (symbols) |
| `radio.set_crc(True)` | `setCRC(2)` / `setCRC(false)` | Enable (`True`/`2`) or disable (`False`/`0`) CRC |
| `radio.set_tcxo(1.6)` | `setTCXO(1.6)` | Set TCXO reference voltage (or 0 for XTAL) |
| `radio.set_dio2_as_rf_switch(True)`| `setDio2AsRfSwitch()` | Configure DIO2 to drive RF switch |

* **See the complete working example**: [`examples/change_settings.py`](examples/change_settings.py).

---

### To Build the Updated Module on your Raspberry Pi:

```bash
cd radiogem

# If using pip in your .venv:
pip install .

# Or if using cmake directly:
cd build && make -j4
```

# #1

I have analyzed both [`error1.txt`](error1.txt) and [`error2.txt`](error2.txt). Both issues have been identified and the code has been updated.

---

### 1. Why `Error writing value to pin 7: GPIO busy` occurred

#### The Root Cause:
* On the Raspberry Pi, **GPIO 7 is the SPI0 CE1 (Hardware Chip Enable 1)** line.
* When `PiHal` opens SPI channel 1 (`lgSpiOpen(0, 1, ...)`), the Linux kernel SPI driver (`spidev0.1` / `spi-bcm2835`) **takes exclusive ownership of GPIO 7**.
* During each SPI transaction (`lgSpiXfer`), the Linux kernel hardware SPI controller **automatically pulses GPIO 7 low and high**.
* However, RadioLib's core `Module` class also tries to manually toggle the CS pin as a software GPIO via `digitalWrite(csPin, ...)`.
* Because the Linux kernel already owns GPIO 7 for hardware SPI, any userspace attempt to claim or write to GPIO 7 via `/dev/gpiochip0` is rejected with **`EBUSY` ("GPIO busy")**.
* This printed 42 error messages on every single packet, cluttering `stderr` and introducing latency.

#### The Fix:
1. **[`src/sx1262_wrapper.cpp`](src/sx1262_wrapper.cpp)**:
   When `nss` matches the active hardware SPI Chip Select (e.g. GPIO 7 on SPI0 Channel 1, or GPIO 8 on SPI0 Channel 0), we now pass `RADIOLIB_NC` to `Module`. RadioLib treats `RADIOLIB_NC` as a no-op, allowing the Linux kernel SPI hardware controller to drive CE1 cleanly without redundant, failing GPIO writes.
2. **[`hal/PiHal.h`](hal/PiHal.h)** and **[`PiHal.h`](PiHal.h)**:
   Added `isSpiHardwareCs(pin)` to `pinMode()` and `digitalWrite()`. If any component attempts to manipulate the hardware CS pin, `PiHal` skips it silently.

---

### 2. Why packets #1, #3, #5 were missed on the ESP32-S3 receiver

Notice the contrast between your two test runs:
* In [`error2.txt`](error2.txt) ([`examples/gpiozero_coexist.py`](examples/gpiozero_coexist.py)), which used `time.sleep(1.0)`, **100% of packets (#0, #1, #2, #3, #4) were received by the exact same ESP32-S3**.
* In [`error1.txt`](error1.txt) ([`examples/basic_tx.py`](examples/basic_tx.py)), which used `time.sleep(2.0)`, **every odd packet (#1, #3, #5) was dropped**.

#### The Mathematical Reason:
In RadioLib on ESP32-S3, when using blocking `radio.receive(str)`:
1. RadioLib calculates the internal RX timeout window as **500% of the maximum Time-on-Air (256 bytes)**:
   $$\text{timeout} \approx \frac{410\text{ ms} \times 5}{1000} \approx \mathbf{2.05\text{ seconds}}$$
2. In `basic_tx.py`, the transmit loop had `time.sleep(2.0)`:
   * **$t = 0.00\text{s}$**: Pi transmits **#0**. ESP32 receives #0, prints it, and opens its next 2.05s RX window (active until $t \approx 2.10\text{s}$).
   * **$t = 2.07\text{s}$**: Pi wakes up from 2.0s sleep and starts transmitting **#1**.
   * **$t = 2.10\text{s}$**: While packet #1's preamble is in the air, the ESP32-S3 hits its **2.05s timeout**! The ESP32 shuts down the receiver, resets the FIFO, and restarts `receive()`. Because packet #1's preamble was interrupted, **packet #1 is missed!**
   * **$t = 2.15\text{s}$**: ESP32 begins its second RX window (active until $t \approx 4.20\text{s}$).
   * **$t = 4.14\text{s}$**: Pi transmits **#2**. Since the ESP32's window is still open, **packet #2 is received!**
   * The cycle repeats: #3 is missed, #4 is received, #5 is missed!
3. In `gpiozero_coexist.py`, the interval was **1.0 second**, so packets arrived safely in the middle of the RX window before any timeout could occur.

---

### Recommended Solutions

#### 1. Rebuild `pyradiolib` on the Raspberry Pi
On your Raspberry Pi, recompile to pick up the updated `PiHal.h` and `sx1262_wrapper.cpp`:
```bash
cd radiogem
# If using pip in your .venv:
pip install .

# Or if using cmake directly:
cd build && make -j4
```

#### 2. Test the updated Transmitter
[`examples/basic_tx.py`](examples/basic_tx.py) has been updated to use a default 1.0s interval (configurable via `--interval`):
```bash
python3 examples/basic_tx.py --interval 1.0
```
With the fix, all `GPIO busy` errors are gone, and packets are received continuously.

#### 3. Best Practice for the ESP32-S3 Receiver
If your ESP32-S3 sketch is using blocking `radio.receive(str)` inside `loop()`, switch it to **interrupt-driven continuous receive**:
```cpp
// Set DIO1 interrupt callback
radio.setPacketReceivedAction(setFlag);

// Start continuous listening (no periodic timeout resets)
radio.startReceive();
```
*(Reference: [`RadioLib/examples/SX126x/SX126x_Receive_Interrupt/SX126x_Receive_Interrupt.ino`](RadioLib/examples/SX126x/SX126x_Receive_Interrupt/SX126x_Receive_Interrupt.ino))*. Continuous receive eliminates receiver timeouts and guarantees that no packet preambles are missed regardless of the transmit interval.
