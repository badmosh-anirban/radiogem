#ifndef RADIOGEM_SX1276_WRAPPER_H
#define RADIOGEM_SX1276_WRAPPER_H

#include "sx127x_wrapper.h"

class SX1276;

/**
 * @brief C++ wrapper around RadioLib's SX1276 LoRa transceiver (137-1020 MHz, 2 to 17/20 dBm).
 */
class SX1276Wrapper : public SX127xWrapper {
public:
    SX1276Wrapper(
        uint8_t spi_channel = 1,
        uint32_t spi_speed = 2000000,
        uint8_t spi_device = 0,
        uint8_t gpio_device = 0,
        int32_t nss = 7,
        int32_t dio0 = 25,
        int32_t reset = 22,
        int32_t dio1 = -1
    );

    virtual ~SX1276Wrapper();

    int16_t begin(
        float frequency = 868.0f,
        float bandwidth = 125.0f,
        uint8_t spreading_factor = 7,
        uint8_t coding_rate = 5,
        uint8_t sync_word = 0x12,
        int8_t power = 10,
        uint16_t preamble_length = 8,
        uint8_t gain = 0
    ) override;

    int16_t set_frequency(float freq) override;
    int16_t set_bandwidth(float bw) override;
    int16_t set_spreading_factor(uint8_t sf) override;
    int16_t set_coding_rate(uint8_t cr) override;
    int16_t set_output_power(int8_t power, bool force_rfo = false) override;
    int16_t set_gain(uint8_t gain) override;
    int16_t set_crc(bool enable, bool mode = false) override;
    void reset() override;

private:
    SX1276* chip = nullptr;
};

#endif // RADIOGEM_SX1276_WRAPPER_H

