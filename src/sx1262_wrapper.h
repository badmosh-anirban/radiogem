#ifndef PYRADIOLIB_SX1262_WRAPPER_H
#define PYRADIOLIB_SX1262_WRAPPER_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <atomic>

// Forward declarations of RadioLib classes
class PiHal;
class Module;
class SX1262;

/**
 * @brief Represents a received LoRa packet with metadata.
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

/**
 * @brief C++ wrapper around RadioLib's SX1262 and Raspberry Pi PiHal.
 */
class SX1262Wrapper {
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

    ~SX1262Wrapper();

    // Prevent copies
    SX1262Wrapper(const SX1262Wrapper&) = delete;
    SX1262Wrapper& operator=(const SX1262Wrapper&) = delete;

    /**
     * @brief Initialize the SX1262 LoRa modem.
     */
    int16_t begin(
        float frequency = 866.5f,
        float bandwidth = 125.0f,
        uint8_t spreading_factor = 7,
        uint8_t coding_rate = 5,
        uint8_t sync_word = 0x12, // RADIOLIB_SX126X_SYNC_WORD_PRIVATE
        int8_t power = 10,
        uint16_t preamble_length = 8,
        float tcxo_voltage = 1.6f,
        bool use_regulator_ldo = false
    );

    // ==========================================
    // 1. Blocking Transmit & Receive
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
    // 2. Non-Blocking / Interrupt Transmit & Receive
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
    // 3. Power & Mode Management
    // ==========================================

    /**
     * @brief Put the radio in standby mode.
     * @param mode 1 = STDBY_RC, 2 = STDBY_XOSC.
     */
    int16_t standby(uint8_t mode = 1);

    /**
     * @brief Put the radio in sleep mode.
     */
    int16_t sleep(bool retain_config = false);

    // ==========================================
    // 4. Runtime RF & Modem Settings
    // ==========================================

    int16_t set_frequency(float freq);
    int16_t set_bandwidth(float bw);
    int16_t set_spreading_factor(uint8_t sf);
    int16_t set_coding_rate(uint8_t cr);
    int16_t set_output_power(int8_t power);
    int16_t set_sync_word(uint8_t sync_word, uint8_t control_bits = 0x44);
    int16_t set_current_limit(float current_limit);
    int16_t set_preamble_length(size_t preamble_length);
    int16_t set_crc(bool enable);
    int16_t set_tcxo(float voltage, uint32_t delay = 5000);
    int16_t set_dio2_as_rf_switch(bool enable = true);

    // Signal quality queries
    float get_rssi();
    float get_snr();

    // Status queries
    int16_t get_last_status() const { return last_status; }
    static std::string get_status_text(int16_t status);

    // Teardown
    void close();

private:
    PiHal* hal = nullptr;
    Module* mod = nullptr;
    SX1262* radio = nullptr;
    int16_t last_status = 0;
    bool is_initialized = false;

    std::atomic<bool> packet_received_flag{false};
    std::atomic<bool> packet_sent_flag{false};
    std::function<void()> py_packet_received_cb = nullptr;
    std::function<void()> py_packet_sent_cb = nullptr;
};

#endif // PYRADIOLIB_SX1262_WRAPPER_H
