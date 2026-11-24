#ifndef RADIOGEM_SX1268_WRAPPER_H
#define RADIOGEM_SX1268_WRAPPER_H

#include "sx126x_wrapper.h"

class SX1268;

/**
 * @brief C++ wrapper around RadioLib's SX1268 LoRa transceiver (410-810 MHz, -9 to +22 dBm).
 */
class SX1268Wrapper : public SX126xWrapper {
public:
    SX1268Wrapper(
        uint8_t spi_channel = 1,
        uint32_t spi_speed = 2000000,
        uint8_t spi_device = 0,
        uint8_t gpio_device = 0,
        int32_t nss = 7,
        int32_t dio1 = 27,
        int32_t reset = 22,
        int32_t busy = 24
    );

    virtual ~SX1268Wrapper();

    /**
     * @brief Initialize the SX1268 LoRa modem.
     */
    int16_t begin(
        float frequency = 434.0f,
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
    SX1268* chip = nullptr;
};

#endif // RADIOGEM_SX1268_WRAPPER_H

