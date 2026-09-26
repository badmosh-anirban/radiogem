I just had few questions in mind, as you know there are 2 different transmit and 2 different types of receive functions in the original radiolib library

# Transmit

## The blocking one

location: RadioLib/examples/SX126x/SX126x_Transmit_Blocking/SX126x_Transmit_Blocking.ino

```cpp
int state = radio.transmit(str)
```

## non blocking/interrupt

location: RadioLib/examples/SX126x/SX126x_Transmit_Interrupt/SX126x_Transmit_Interrupt.ino

```cpp
// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
// and MUST NOT have any arguments!

void setFlag(void) {
// we sent a packet, set the flag
transmittedFlag = true;
}

// set the function that will be called
// when packet transmission is finished
radio.setPacketSentAction(setFlag);

transmissionState = radio.startTransmit(str);

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();
```

# Receive

## The blocking

location: RadioLib/examples/SX126x/SX126x_Receive_Blocking/SX126x_Receive_Blocking.ino

```cpp
int state = radio.receive(str);
```

## non blocking/interrupt

location: RadioLib/examples/SX126x/SX126x_Receive_Interrupt/SX126x_Receive_Interrupt.ino

```cpp
// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
// and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
// we got a packet, set the flag
receivedFlag = true;
}

radio.setPacketReceivedAction(setFlag);

//in setup
state = radio.startReceive();

//in loop
int state = radio.readData(str);
```

# Other settings

I think these can be changed even while the code is running <br>
location: RadioLib/examples/SX126x/SX126x_Settings/SX126x_Settings.ino

```cpp
// you can also change the settings at runtime
// and check if the configuration was changed successfully

// set carrier frequency to 433.5 MHz
if (radio1.setFrequency(433.5) == RADIOLIB_ERR_INVALID_FREQUENCY) {
Serial.println(F("Selected frequency is invalid for this module!"));
while (true) { delay(10); }
}

// set bandwidth to 250 kHz
if (radio1.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
Serial.println(F("Selected bandwidth is invalid for this module!"));
while (true) { delay(10); }
}

// set spreading factor to 10
if (radio1.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
Serial.println(F("Selected spreading factor is invalid for this module!"));
while (true) { delay(10); }
}

// set coding rate to 6
if (radio1.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
Serial.println(F("Selected coding rate is invalid for this module!"));
while (true) { delay(10); }
}

// set LoRa sync word to 0xAB
if (radio1.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
Serial.println(F("Unable to set sync word!"));
while (true) { delay(10); }
}

// set output power to 10 dBm (accepted range is -17 - 22 dBm)
if (radio1.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
Serial.println(F("Selected output power is invalid for this module!"));
while (true) { delay(10); }
}

// set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
// NOTE: set value to 0 to disable overcurrent protection
if (radio1.setCurrentLimit(80) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
Serial.println(F("Selected current limit is invalid for this module!"));
while (true) { delay(10); }
}

// set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
if (radio1.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
Serial.println(F("Selected preamble length is invalid for this module!"));
while (true) { delay(10); }
}

// disable CRC
if (radio1.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
Serial.println(F("Selected CRC is invalid for this module!"));
while (true) { delay(10); }
}

// Some SX126x modules have TCXO (temperature compensated crystal
// oscillator). To configure TCXO reference voltage,
// the following method can be used.
if (radio1.setTCXO(2.4) == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE) {
Serial.println(F("Selected TCXO voltage is invalid for this module!"));
while (true) { delay(10); }
}

// Some SX126x modules use DIO2 as RF switch. To enable
// this feature, the following method can be used.
// NOTE: As long as DIO2 is configured to control RF switch,
// it can't be used as interrupt pin!
if (radio1.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
Serial.println(F("Failed to set DIO2 as RF switch!"));
while (true) { delay(10); }
}

Serial.println(F("All settings succesfully changed!"));
}

void loop() {
// nothing here
}
```

So have you implemented all of these. Atleast tell me how are you going to implement the two different types of receive and transmit functions respectively in Python, or have you thought of any kind of different approach when converting the cpp to Python.

And for the other settings section, don't you think it would be good to implement these features too?
