#include "sx1262_wrapper.h"
#include "PiHal.h"
#include <RadioLib.h>
#include <cstdio>

SX1262Wrapper::SX1262Wrapper(
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

    hal = new PiHal(spi_channel, spi_speed, spi_device, gpio_device);
    mod = new Module(hal, nss_pin, dio1_pin, rst_pin, busy_pin);
    radio = new SX1262(mod);
}

SX1262Wrapper::~SX1262Wrapper() {
    close();
}

void SX1262Wrapper::close() {
    if (radio) {
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

int16_t SX1262Wrapper::begin(
    float frequency,
    float bandwidth,
    uint8_t spreading_factor,
    uint8_t coding_rate,
    uint8_t sync_word,
    int8_t power,
    uint16_t preamble_length,
    float tcxo_voltage,
    bool use_regulator_ldo
) {
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }

    last_status = radio->begin(
        frequency,
        bandwidth,
        spreading_factor,
        coding_rate,
        sync_word,
        power,
        preamble_length,
        tcxo_voltage,
        use_regulator_ldo
    );

    if (last_status == RADIOLIB_ERR_NONE) {
        is_initialized = true;
    }
    return last_status;
}

int16_t SX1262Wrapper::transmit(const std::string& data) {
    return transmit_raw(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

int16_t SX1262Wrapper::transmit_raw(const uint8_t* data, size_t len) {
    if (!radio) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }
    last_status = radio->transmit(data, len);
    return last_status;
}

std::optional<Packet> SX1262Wrapper::receive(uint32_t timeout_ms, bool return_none_on_error) {
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

int16_t SX1262Wrapper::standby(uint8_t mode) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->standby(mode);
    return last_status;
}

int16_t SX1262Wrapper::sleep(bool retain_config) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->sleep(retain_config);
    return last_status;
}

int16_t SX1262Wrapper::set_frequency(float freq) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setFrequency(freq);
    return last_status;
}

int16_t SX1262Wrapper::set_bandwidth(float bw) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setBandwidth(bw);
    return last_status;
}

int16_t SX1262Wrapper::set_spreading_factor(uint8_t sf) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setSpreadingFactor(sf);
    return last_status;
}

int16_t SX1262Wrapper::set_coding_rate(uint8_t cr) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setCodingRate(cr);
    return last_status;
}

int16_t SX1262Wrapper::set_output_power(int8_t power) {
    if (!radio) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = radio->setOutputPower(power);
    return last_status;
}

float SX1262Wrapper::get_rssi() {
    if (!radio) return 0.0f;
    return radio->getRSSI();
}

float SX1262Wrapper::get_snr() {
    if (!radio) return 0.0f;
    return radio->getSNR();
}

std::string SX1262Wrapper::get_status_text(int16_t status) {
    switch (status) {
        case RADIOLIB_ERR_NONE:
            return "SUCCESS (0): Method executed successfully";
        case RADIOLIB_ERR_UNKNOWN:
            return "UNKNOWN_ERROR (-1): Unexpected error occurred";
        case RADIOLIB_ERR_CHIP_NOT_FOUND:
            return "CHIP_NOT_FOUND (-2): Radio chip not responding; check SPI wiring, channel, and NSS pin";
        case RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED:
            return "MEMORY_ALLOCATION_FAILED (-3): Buffer memory allocation failed";
        case RADIOLIB_ERR_PACKET_TOO_LONG:
            return "PACKET_TOO_LONG (-4): Packet length exceeds maximum supported by radio";
        case RADIOLIB_ERR_TX_TIMEOUT:
            return "TX_TIMEOUT (-5): Transmit operation timed out";
        case RADIOLIB_ERR_RX_TIMEOUT:
            return "RX_TIMEOUT (-6): Timed out waiting for incoming packet";
        case RADIOLIB_ERR_CRC_MISMATCH:
            return "CRC_MISMATCH (-7): Received packet corrupted (CRC check failed)";
        case RADIOLIB_ERR_INVALID_BANDWIDTH:
            return "INVALID_BANDWIDTH (-8): Specified bandwidth is invalid for SX1262";
        case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
            return "INVALID_SPREADING_FACTOR (-9): Specified spreading factor is invalid for SX1262";
        case RADIOLIB_ERR_INVALID_CODING_RATE:
            return "INVALID_CODING_RATE (-10): Specified coding rate is invalid for SX1262";
        case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
            return "INVALID_OUTPUT_POWER (-13): Specified output power is out of valid range";
        case RADIOLIB_ERR_SPI_CMD_TIMEOUT:
            return "SPI_CMD_TIMEOUT (-706): SX126x command timed out (check TCXO vs XTAL setting, or BUSY pin)";
        case RADIOLIB_ERR_SPI_CMD_INVALID:
            return "SPI_CMD_INVALID (-707): SX126x command invalid (check TCXO vs XTAL setting)";
        case RADIOLIB_ERR_SPI_CMD_FAILED:
            return "SPI_CMD_FAILED (-708): SX126x command failed";
        default:
            return "ERROR (" + std::to_string(status) + ")";
    }
}
