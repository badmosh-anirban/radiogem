#ifndef RADIOGEM_PACKET_H
#define RADIOGEM_PACKET_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief Represents a received LoRa packet with payload data and signal metrics.
 */
struct Packet {
    std::vector<uint8_t> data;
    float rssi = 0.0f;
    float snr = 0.0f;
    int16_t status = 0;

    std::string text() const {
        return std::string(data.begin(), data.end());
    }

    size_t size() const {
        return data.size();
    }

    bool is_valid() const {
        return status == 0;
    }
};

#endif // RADIOGEM_PACKET_H

