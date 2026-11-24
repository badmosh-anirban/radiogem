#include "sx1262_wrapper.h"
#include <RadioLib.h>

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
    init_hardware(spi_channel, spi_speed, spi_device, gpio_device, nss, dio1, reset, busy);
    chip = new SX1262(mod);
    radio = chip;
}

SX1262Wrapper::~SX1262Wrapper() {
    close();
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
        tcxo_voltage,
        use_regulator_ldo
    );

    if (last_status == RADIOLIB_ERR_NONE) {
        is_initialized = true;
    }
    return last_status;
}

