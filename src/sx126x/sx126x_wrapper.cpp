#include "sx126x_wrapper.h"
#include "PiHal.h"
#include <RadioLib.h>
#include <cstdio>

static SX126xWrapper* s_active_instance = nullptr;

static void onPacketReceivedStatic() {
    if (s_active_instance) {
        s_active_instance->handlePacketReceived();
    }
}

static void onPacketSentStatic() {
    if (s_active_instance) {
        s_active_instance->handlePacketSent();
    }
}

SX126xWrapper::SX126xWrapper() {
}

SX126xWrapper::~SX126xWrapper() {
    if (s_active_instance == this) {
        s_active_instance = nullptr;
    }
    close();
}

void SX126xWrapper::init_hardware(
    uint8_t spi_channel,
    uint32_t spi_speed,
    uint8_t spi_device,
    uint8_t gpio_device,
    int32_t nss,
    int32_t dio1,
    int32_t reset,
    int32_t busy
) {
    uint32_t nss_pin  = (nss < 0)   ? RADIOLIB_NC : static_cast<uint32_t>(nss);
    uint32_t dio1_pin = (dio1 < 0)  ? RADIOLIB_NC : static_cast<uint32_t>(dio1);
    uint32_t rst_pin  = (reset < 0) ? RADIOLIB_NC : static_cast<uint32_t>(reset);
    uint32_t busy_pin = (busy < 0)  ? RADIOLIB_NC : static_cast<uint32_t>(busy);

    // If NSS matches the hardware SPI Chip Enable pin for the given SPI bus,
    // the Linux kernel SPI driver (spidev) asserts and deasserts the pin automatically
    // during lgSpiXfer(). Passing RADIOLIB_NC to Module tells RadioLib not to
    // redundantly attempt manual software GPIO writes to a kernel-claimed pin.
    bool is_hw_cs = false;
    if (spi_device == 0) {
        if ((spi_channel == 0 && nss_pin == 8) || (spi_channel == 1 && nss_pin == 7)) {
            is_hw_cs = true;
        }
    } else if (spi_device == 1) {
        if ((spi_channel == 0 && nss_pin == 18) || (spi_channel == 1 && nss_pin == 17) || (spi_channel == 2 && nss_pin == 16)) {
            is_hw_cs = true;
        }
    }
    uint32_t mod_nss = is_hw_cs ? RADIOLIB_NC : nss_pin;

    hal = new PiHal(spi_channel, spi_speed, spi_device, gpio_device);
    mod = new Module(hal, mod_nss, dio1_pin, rst_pin, busy_pin);
}

void SX126xWrapper::close() {
    if (s_active_instance == this) {
        s_active_instance = nullptr;
    }
    if (radio) {
        clear_packet_received_action();
        clear_packet_sent_action();
        delete radio;
        radio = nullptr;
    }
    if (mod) {
        delete mod;
        mod = nullptr;
    }
    if (hal) {
        hal->term();
        delete hal;
        hal = nullptr;
    }
    is_initialized = false;
}

// ==========================================
// 2. Blocking Transmit & Receive
// ==========================================

int16_t SX126xWrapper::transmit(const std::string& data) {
    return transmit_raw(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

int16_t SX126xWrapper::transmit_raw(const uint8_t* data, size_t len) {
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }
    last_status = radio->transmit(data, len);
    return last_status;
}

std::optional<Packet> SX126xWrapper::receive(uint32_t timeout_ms, bool return_none_on_error) {
    Packet pkt;
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        pkt.status = last_status;
        if (return_none_on_error) return std::nullopt;
        return pkt;
    }

    uint8_t buffer[RADIOLIB_SX126X_MAX_PACKET_LENGTH] = { 0 };
    int16_t state = radio->receive(buffer, 0, timeout_ms);
    pkt.status = state;
    last_status = state;

    if (state == RADIOLIB_ERR_NONE) {
        size_t len = radio->getPacketLength(false);
        pkt.data.assign(buffer, buffer + len);
        pkt.rssi = radio->getRSSI();
        pkt.snr = radio->getSNR();
        return pkt;
    }

    if (return_none_on_error) {
        return std::nullopt;
    }
    return pkt;
}

// ==========================================
// 3. Non-Blocking / Interrupt Transmit & Receive
// ==========================================

int16_t SX126xWrapper::start_transmit(const std::string& data) {
    return start_transmit_raw(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

int16_t SX126xWrapper::start_transmit_raw(const uint8_t* data, size_t len) {
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }
    packet_sent_flag = false;
    last_status = radio->startTransmit(data, len);
    return last_status;
}

int16_t SX126xWrapper::finish_transmit() {
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }
    packet_sent_flag = false;
    last_status = radio->finishTransmit();
    return last_status;
}

int16_t SX126xWrapper::start_receive(uint32_t timeout_ms) {
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }
    packet_received_flag = false;
    if (timeout_ms == 0) {
        last_status = radio->startReceive(); // Continuous RX mode
    } else {
        uint32_t timeoutValue = (uint32_t)(((float)timeout_ms * 1000.0f) / 15.625f);
        last_status = radio->startReceive(timeoutValue);
    }
    return last_status;
}

std::optional<Packet> SX126xWrapper::read_data(bool return_none_on_error) {
    Packet pkt;
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        pkt.status = last_status;
        if (return_none_on_error) return std::nullopt;
        return pkt;
    }

    uint8_t buffer[RADIOLIB_SX126X_MAX_PACKET_LENGTH] = { 0 };
    int16_t state = radio->readData(buffer, 0);
    pkt.status = state;
    last_status = state;

    if (state == RADIOLIB_ERR_NONE) {
        size_t len = radio->getPacketLength(false);
        pkt.data.assign(buffer, buffer + len);
        pkt.rssi = radio->getRSSI();
        pkt.snr = radio->getSNR();
        packet_received_flag = false;
        return pkt;
    }

    if (return_none_on_error) {
        return std::nullopt;
    }
    return pkt;
}

void SX126xWrapper::handlePacketReceived() {
    packet_received_flag = true;
    if (py_packet_received_cb) {
        py_packet_received_cb();
    }
}

void SX126xWrapper::handlePacketSent() {
    packet_sent_flag = true;
    if (py_packet_sent_cb) {
        py_packet_sent_cb();
    }
}

void SX126xWrapper::set_packet_received_action(std::function<void()> cb) {
    py_packet_received_cb = cb;
    packet_received_flag = false;
    s_active_instance = this;
    if (radio) {
        radio->setPacketReceivedAction(onPacketReceivedStatic);
    }
}

void SX126xWrapper::clear_packet_received_action() {
    py_packet_received_cb = nullptr;
    if (radio) {
        radio->clearPacketReceivedAction();
    }
}

void SX126xWrapper::set_packet_sent_action(std::function<void()> cb) {
    py_packet_sent_cb = cb;
    packet_sent_flag = false;
    s_active_instance = this;
    if (radio) {
        radio->setPacketSentAction(onPacketSentStatic);
    }
}

void SX126xWrapper::clear_packet_sent_action() {
    py_packet_sent_cb = nullptr;
    if (radio) {
        radio->clearPacketSentAction();
    }
}

// ==========================================
// 4. Power & Mode Management
// ==========================================

int16_t SX126xWrapper::standby(uint8_t mode) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->standby(mode);
    return last_status;
}

int16_t SX126xWrapper::sleep(bool retain_config) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->sleep(retain_config);
    return last_status;
}

// ==========================================
// 5. Runtime RF & Modem Settings
// ==========================================

int16_t SX126xWrapper::set_frequency(float freq) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setFrequency(freq);
    return last_status;
}

int16_t SX126xWrapper::set_bandwidth(float bw) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setBandwidth(bw);
    return last_status;
}

int16_t SX126xWrapper::set_spreading_factor(uint8_t sf) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setSpreadingFactor(sf);
    return last_status;
}

int16_t SX126xWrapper::set_coding_rate(uint8_t cr) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setCodingRate(cr);
    return last_status;
}

int16_t SX126xWrapper::set_output_power(int8_t power) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setOutputPower(power);
    return last_status;
}

int16_t SX126xWrapper::set_sync_word(uint8_t sync_word, uint8_t control_bits) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setSyncWord(sync_word, control_bits);
    return last_status;
}

int16_t SX126xWrapper::set_current_limit(float current_limit) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setCurrentLimit(current_limit);
    return last_status;
}

int16_t SX126xWrapper::set_preamble_length(size_t preamble_length) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setPreambleLength(preamble_length);
    return last_status;
}

int16_t SX126xWrapper::set_crc(bool enable) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setCRC(enable ? 2 : 0);
    return last_status;
}

int16_t SX126xWrapper::set_tcxo(float voltage, uint32_t delay) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setTCXO(voltage, delay);
    return last_status;
}

int16_t SX126xWrapper::set_dio2_as_rf_switch(bool enable) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setDio2AsRfSwitch(enable);
    return last_status;
}

float SX126xWrapper::get_rssi() {
    if (!radio) return 0.0f;
    return radio->getRSSI();
}

float SX126xWrapper::get_snr() {
    if (!radio) return 0.0f;
    return radio->getSNR();
}

