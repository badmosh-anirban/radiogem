#include "sx1277_wrapper.h"
#include <RadioLib.h>

SX1277Wrapper::SX1277Wrapper(
    uint8_t spi_channel,
    uint32_t spi_speed,
    uint8_t spi_device,
    uint8_t gpio_device,
    int32_t nss,
    int32_t dio0,
    int32_t reset,
    int32_t dio1
) {
    init_hardware(spi_channel, spi_speed, spi_device, gpio_device, nss, dio0, reset, dio1);
    chip = new SX1277(mod);
    radio = chip;
}

SX1277Wrapper::~SX1277Wrapper() {
    close();
}

int16_t SX1277Wrapper::begin(
    float frequency,
    float bandwidth,
    uint8_t spreading_factor,
    uint8_t coding_rate,
    uint8_t sync_word,
    int8_t power,
    uint16_t preamble_length,
    uint8_t gain
) {
    if (!chip) {
        last_status = RADIOLIB_ERR_CHIP_NOT_FOUND;
        return last_status;
    }

    last_status = chip->begin(
        frequency,
        bandwidth,
        spreading_factor,
        coding_rate,
        sync_word,
        power,
        preamble_length,
        gain
    );

    if (last_status == RADIOLIB_ERR_NONE) {
        is_initialized = true;
    }
    return last_status;
}

int16_t SX1277Wrapper::set_frequency(float freq) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setFrequency(freq);
    return last_status;
}

int16_t SX1277Wrapper::set_bandwidth(float bw) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setBandwidth(bw);
    return last_status;
}

int16_t SX1277Wrapper::set_spreading_factor(uint8_t sf) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setSpreadingFactor(sf);
    return last_status;
}

int16_t SX1277Wrapper::set_coding_rate(uint8_t cr) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setCodingRate(cr);
    return last_status;
}

int16_t SX1277Wrapper::set_output_power(int8_t power, bool force_rfo) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setOutputPower(power, force_rfo);
    return last_status;
}

int16_t SX1277Wrapper::set_gain(uint8_t gain) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setGain(gain);
    return last_status;
}

int16_t SX1277Wrapper::set_crc(bool enable, bool mode) {
    if (!chip) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    last_status = chip->setCRC(enable, mode);
    return last_status;
}

void SX1277Wrapper::reset() {
    if (chip) {
        chip->reset();
    }
}

