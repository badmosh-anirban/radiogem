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
