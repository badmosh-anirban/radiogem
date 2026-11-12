#ifndef RADIOGEM_SX127X_WRAPPER_H
#define RADIOGEM_SX127X_WRAPPER_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <atomic>
#include "packet.h"

// Forward declarations of RadioLib and HAL classes
class PiHal;
class Module;
class SX127x;

/**
 * @brief Base C++ wrapper for Semtech 1st generation SX127x LoRa radios
 *        (SX1272, SX1273, SX1276, SX1277, SX1278, SX1279) on Raspberry Pi / Linux using lgpio.
 */
class SX127xWrapper {
public:
    SX127xWrapper();
    virtual ~SX127xWrapper();

    // Prevent copies
    SX127xWrapper(const SX127xWrapper&) = delete;
    SX127xWrapper& operator=(const SX127xWrapper&) = delete;

    // ==========================================
    // 1. Initialization (pure virtual in base)
    // ==========================================
    virtual int16_t begin(
        float frequency,
        float bandwidth,
        uint8_t spreading_factor,
        uint8_t coding_rate,
        uint8_t sync_word,
        int8_t power,
        uint16_t preamble_length,
        uint8_t gain
    ) = 0;

    // ==========================================
    // 2. Blocking Transmit & Receive
    // ==========================================

    /**
     * @brief Transmit string data (blocking until completion).
     */
    int16_t transmit(const std::string& data);

    /**
     * @brief Transmit raw binary data (blocking until completion).
     */
    int16_t transmit_raw(const uint8_t* data, size_t len);

    /**
     * @brief Receive a LoRa packet (blocking with timeout).
     * @param timeout_ms Timeout in milliseconds. 0 = calculate default timeout based on time-on-air.
     * @param return_none_on_error If true, returns std::nullopt on timeout or error.
     */
    std::optional<Packet> receive(uint32_t timeout_ms = 0, bool return_none_on_error = true);

    // ==========================================
    // 3. Non-Blocking / Interrupt Transmit & Receive
    // ==========================================

    /**
     * @brief Start non-blocking transmission of string data.
     */
    int16_t start_transmit(const std::string& data);

    /**
     * @brief Start non-blocking transmission of binary data.
     */
    int16_t start_transmit_raw(const uint8_t* data, size_t len);

    /**
     * @brief Finish non-blocking transmission (cleans up transmitter, standby mode).
     */
    int16_t finish_transmit();

    /**
     * @brief Start non-blocking listening for packets (continuous RX if timeout_ms=0).
     */
    int16_t start_receive(uint32_t timeout_ms = 0);

    /**
     * @brief Read data received by the radio after an interrupt.
     */
    std::optional<Packet> read_data(bool return_none_on_error = true);

    /**
     * @brief Set callback to be invoked when a complete packet is received.
     */
    void set_packet_received_action(std::function<void()> cb);
    void clear_packet_received_action();

    /**
     * @brief Set callback to be invoked when packet transmission is finished.
     */
    void set_packet_sent_action(std::function<void()> cb);
    void clear_packet_sent_action();

    /**
     * @brief Interrupt status flags for polling-based workflows.
     */
    bool has_received() const { return packet_received_flag.load(); }
    bool has_sent() const { return packet_sent_flag.load(); }
    void clear_flags() { packet_received_flag = false; packet_sent_flag = false; }

    // Internal handlers called from C ISRs
    void handlePacketReceived();
    void handlePacketSent();

    // ==========================================
    // 4. Power & Mode Management
    // ==========================================

    /**
     * @brief Put the radio in standby mode.
     * @param mode 1 = STDBY (default on SX127x).
     */
    int16_t standby(uint8_t mode = 1);

    /**
     * @brief Put the radio in sleep mode.
     */
    int16_t sleep();

    /**
     * @brief Hardware reset the radio chip.
     */
    virtual void reset() = 0;

    // ==========================================
    // 5. Runtime RF & Modem Settings
    // ==========================================

    virtual int16_t set_frequency(float freq) = 0;
    virtual int16_t set_bandwidth(float bw) = 0;
    virtual int16_t set_spreading_factor(uint8_t sf) = 0;
    virtual int16_t set_coding_rate(uint8_t cr) = 0;
    virtual int16_t set_output_power(int8_t power, bool force_rfo = false) = 0;
    virtual int16_t set_gain(uint8_t gain) = 0;
    virtual int16_t set_crc(bool enable, bool mode = false) = 0;

    int16_t set_sync_word(uint8_t sync_word);
    int16_t set_current_limit(uint8_t current_limit);
    int16_t set_preamble_length(size_t preamble_length);

    // Signal quality queries
    float get_rssi();
    float get_snr();

    // Status queries
    int16_t get_last_status() const { return last_status; }

    // Teardown
    void close();

protected:
    void init_hardware(
        uint8_t spi_channel,
        uint32_t spi_speed,
        uint8_t spi_device,
        uint8_t gpio_device,
        int32_t nss,
        int32_t dio0,
        int32_t reset,
        int32_t dio1
    );

    PiHal* hal = nullptr;
    Module* mod = nullptr;
    SX127x* radio = nullptr;
    int16_t last_status = 0;
    bool is_initialized = false;

    std::atomic<bool> packet_received_flag{false};
    std::atomic<bool> packet_sent_flag{false};
    std::function<void()> py_packet_received_cb = nullptr;
    std::function<void()> py_packet_sent_cb = nullptr;
};

#endif // RADIOGEM_SX127X_WRAPPER_H

