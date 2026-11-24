#ifndef RADIOGEM_SX1262_WRAPPER_H
#define RADIOGEM_SX1262_WRAPPER_H

#include "sx126x_wrapper.h"

class SX1262;

/**
 * @brief C++ wrapper around RadioLib's SX1262 LoRa transceiver (-9 to +22 dBm).
 */
class SX1262Wrapper : public SX126xWrapper {
public:
    SX1262Wrapper(
        uint8_t spi_channel = 1,
        uint32_t spi_speed = 2000000,
        uint8_t spi_device = 0,
        uint8_t gpio_device = 0,
        int32_t nss = 7,
        int32_t dio1 = 27,
        int32_t reset = 22,
        int32_t busy = 24
    );

    virtual ~SX1262Wrapper();

    /**
     * @brief Initialize the SX1262 LoRa modem.
     */
    int16_t begin(
        float frequency = 866.5f,
        float bandwidth = 125.0f,
        uint8_t spreading_factor = 7,
        uint8_t coding_rate = 5,
        uint8_t sync_word = 0x12,
        int8_t power = 10,
        uint16_t preamble_length = 8,
        float tcxo_voltage = 1.6f,
        bool use_regulator_ldo = false
    ) override;

private:
    SX1262* chip = nullptr;
};

#endif // RADIOGEM_SX1262_WRAPPER_H

