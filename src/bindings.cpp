#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "sx1262_wrapper.h"

namespace py = pybind11;

PYBIND11_MODULE(pyradiolib, m) {
    m.doc() = "Python bindings for RadioLib SX1262 on Raspberry Pi";

    // RadioLib Status Codes
    m.attr("ERR_NONE")                      = 0;
    m.attr("ERR_UNKNOWN")                   = -1;
    m.attr("ERR_CHIP_NOT_FOUND")            = -2;
    m.attr("ERR_MEMORY_ALLOCATION_FAILED")  = -3;
    m.attr("ERR_PACKET_TOO_LONG")           = -4;
    m.attr("ERR_TX_TIMEOUT")                = -5;
    m.attr("ERR_RX_TIMEOUT")                = -6;
    m.attr("ERR_CRC_MISMATCH")              = -7;
    m.attr("ERR_INVALID_BANDWIDTH")         = -8;
    m.attr("ERR_INVALID_SPREADING_FACTOR")  = -9;
    m.attr("ERR_INVALID_CODING_RATE")       = -10;
    m.attr("ERR_INVALID_OUTPUT_POWER")      = -13;
    m.attr("ERR_INVALID_CRC_CONFIGURATION") = -14;
    m.attr("ERR_INVALID_CURRENT_LIMIT")     = -17;
    m.attr("ERR_INVALID_PREAMBLE_LENGTH")   = -18;
    m.attr("ERR_INVALID_TCXO_VOLTAGE")      = -705;
    m.attr("ERR_SPI_CMD_TIMEOUT")           = -706;
    m.attr("ERR_SPI_CMD_INVALID")           = -707;
    m.attr("ERR_SPI_CMD_FAILED")            = -708;

    m.def("get_status_text", &SX1262Wrapper::get_status_text,
          py::arg("status"),
          "Convert a RadioLib integer status code to human-readable text");

    // Packet class
    py::class_<Packet>(m, "Packet", "A received LoRa packet containing payload and signal metrics")
        .def_property_readonly("payload", [](const Packet& p) {
            return py::bytes(reinterpret_cast<const char*>(p.data.data()), p.data.size());
        }, "Raw payload as bytes")
        .def_property_readonly("data", [](const Packet& p) {
            return py::bytes(reinterpret_cast<const char*>(p.data.data()), p.data.size());
        }, "Alias for payload as bytes")
        .def_property_readonly("text", &Packet::text, "Payload decoded as a UTF-8 string")
        .def_readonly("rssi", &Packet::rssi, "Received Signal Strength Indicator (RSSI) in dBm")
        .def_readonly("snr", &Packet::snr, "Signal-to-Noise Ratio (SNR) in dB")
        .def_readonly("status", &Packet::status, "Status code (0 = success)")
        .def("__len__", &Packet::size)
        .def("__bool__", &Packet::is_valid)
        .def("__bytes__", [](const Packet& p) {
            return py::bytes(reinterpret_cast<const char*>(p.data.data()), p.data.size());
        })
        .def("__str__", &Packet::text)
        .def("__repr__", [](const Packet& p) {
            if (p.is_valid()) {
                std::string s(p.data.begin(), p.data.end());
                return "<Packet len=" + std::to_string(p.data.size()) +
                       " payload=\"" + s + "\" rssi=" + std::to_string(p.rssi) +
                       "dBm snr=" + std::to_string(p.snr) + "dB>";
            } else {
                return "<Packet status=" + std::to_string(p.status) + " (" +
                       SX1262Wrapper::get_status_text(p.status) + ")>";
            }
        });

    // SX1262 Radio Class
    py::class_<SX1262Wrapper>(m, "SX1262", "RadioLib SX1262 driver for Raspberry Pi")
        .def(py::init<uint8_t, uint32_t, uint8_t, uint8_t, int32_t, int32_t, int32_t, int32_t>(),
            py::arg("spi_channel") = 1,
            py::arg("spi_speed") = 2000000,
            py::arg("spi_device") = 0,
            py::arg("gpio_device") = 0,
            py::arg("nss") = 7,
            py::arg("dio1") = 17,
            py::arg("reset") = 22,
            py::arg("busy") = 24,
            "Instantiate SX1262 radio with SPI and GPIO pin assignments")
        .def("begin", &SX1262Wrapper::begin,
            py::arg("frequency") = 866.5f,
            py::arg("bandwidth") = 125.0f,
            py::arg("spreading_factor") = 7,
            py::arg("coding_rate") = 5,
            py::arg("sync_word") = 0x12,
            py::arg("power") = 10,
            py::arg("preamble_length") = 8,
            py::arg("tcxo_voltage") = 1.6f,
            py::arg("use_regulator_ldo") = false,
            "Initialize SX1262 modem for LoRa. Returns status code (0 = success)")

        // ---------------------------------------------------------------------
        // 1. Blocking Transmit & Receive
        // ---------------------------------------------------------------------
        .def("transmit", [](SX1262Wrapper& self, const py::object& data) {
            if (py::isinstance<py::bytes>(data)) {
                std::string s = data.cast<std::string>();
                return self.transmit_raw(reinterpret_cast<const uint8_t*>(s.data()), s.size());
            } else if (py::isinstance<py::str>(data)) {
                std::string s = data.cast<std::string>();
                return self.transmit(s);
            } else {
                throw py::type_error("transmit() data must be str or bytes");
            }
        }, py::call_guard<py::gil_scoped_release>(),
           py::arg("data"),
           "Transmit a packet (blocking). Releases GIL during transmission. Returns status code (0 = success)")
        .def("receive", &SX1262Wrapper::receive,
            py::call_guard<py::gil_scoped_release>(),
            py::arg("timeout_ms") = 0,
            py::arg("return_none_on_error") = true,
            "Receive a LoRa packet (blocking with timeout). Releases GIL. Returns Packet or None")

        // ---------------------------------------------------------------------
        // 2. Non-Blocking / Interrupt Transmit & Receive
        // ---------------------------------------------------------------------
        .def("start_transmit", [](SX1262Wrapper& self, const py::object& data) {
            if (py::isinstance<py::bytes>(data)) {
                std::string s = data.cast<std::string>();
                return self.start_transmit_raw(reinterpret_cast<const uint8_t*>(s.data()), s.size());
            } else if (py::isinstance<py::str>(data)) {
                std::string s = data.cast<std::string>();
                return self.start_transmit(s);
            } else {
                throw py::type_error("start_transmit() data must be str or bytes");
            }
        }, py::arg("data"), "Start non-blocking transmission (interrupt triggers when done). Returns status code (0 = success)")
        .def("finish_transmit", &SX1262Wrapper::finish_transmit,
            "Clean up transmitter after transmission is finished (powers down RF switch, puts radio in standby)")
        .def("start_receive", &SX1262Wrapper::start_receive,
            py::arg("timeout_ms") = 0,
            "Start non-blocking packet reception (0 = continuous listen mode until packet arrives). Returns status code (0 = success)")
        .def("read_data", &SX1262Wrapper::read_data,
            py::arg("return_none_on_error") = true,
            "Read packet data after receiving an interrupt alert. Returns Packet or None")
        .def("set_packet_received_action", [](SX1262Wrapper& self, py::function func) {
            self.set_packet_received_action([func]() {
                py::gil_scoped_acquire acquire;
                try {
                    func();
                } catch (const py::error_already_set& e) {
                    fprintf(stderr, "Exception in Python packet_received callback: %s\n", e.what());
                }
            });
        }, py::arg("callback"), "Set a Python callback function to be called when a complete packet is received via interrupt")
        .def("clear_packet_received_action", &SX1262Wrapper::clear_packet_received_action,
            "Clear the packet received callback")
        .def("set_packet_sent_action", [](SX1262Wrapper& self, py::function func) {
            self.set_packet_sent_action([func]() {
                py::gil_scoped_acquire acquire;
                try {
                    func();
                } catch (const py::error_already_set& e) {
                    fprintf(stderr, "Exception in Python packet_sent callback: %s\n", e.what());
                }
            });
        }, py::arg("callback"), "Set a Python callback function to be called when packet transmission finishes via interrupt")
        .def("clear_packet_sent_action", &SX1262Wrapper::clear_packet_sent_action,
            "Clear the packet sent callback")
        .def_property_readonly("has_received", &SX1262Wrapper::has_received,
            "True if a packet has been received via interrupt since last read")
        .def_property_readonly("has_sent", &SX1262Wrapper::has_sent,
            "True if packet transmission finished via interrupt")
        .def("clear_flags", &SX1262Wrapper::clear_flags,
            "Reset internal interrupt status flags")

        // ---------------------------------------------------------------------
        // 3. Power & Mode Management
        // ---------------------------------------------------------------------
        .def("standby", &SX1262Wrapper::standby,
            py::arg("mode") = 1,
            "Enter standby mode (1 = STDBY_RC, 2 = STDBY_XOSC)")
        .def("sleep", &SX1262Wrapper::sleep,
            py::arg("retain_config") = false,
            "Enter low-power sleep mode")

        // ---------------------------------------------------------------------
        // 4. Runtime RF & Modem Settings
        // ---------------------------------------------------------------------
        .def("set_frequency", &SX1262Wrapper::set_frequency,
            py::arg("freq"),
            "Change carrier frequency in MHz (e.g. 866.5)")
        .def("set_bandwidth", &SX1262Wrapper::set_bandwidth,
            py::arg("bw"),
            "Change LoRa bandwidth in kHz (e.g. 125.0, 250.0, 500.0)")
        .def("set_spreading_factor", &SX1262Wrapper::set_spreading_factor,
            py::arg("sf"),
            "Change LoRa spreading factor (5 - 12)")
        .def("set_coding_rate", &SX1262Wrapper::set_coding_rate,
            py::arg("cr"),
            "Change LoRa coding rate denominator (5 - 8 for 4/5 - 4/8)")
        .def("set_output_power", &SX1262Wrapper::set_output_power,
            py::arg("power"),
            "Change transmission output power in dBm (-9 to +22 dBm)")
        .def("set_sync_word", &SX1262Wrapper::set_sync_word,
            py::arg("sync_word"),
            py::arg("control_bits") = 0x44,
            "Set 1-byte LoRa sync word (e.g. 0x12 for private, 0x34 for public/LoRaWAN)")
        .def("set_current_limit", &SX1262Wrapper::set_current_limit,
            py::arg("current_limit"),
            "Set over-current protection limit in mA (range: 45 - 240 mA; 0 to disable)")
        .def("set_preamble_length", &SX1262Wrapper::set_preamble_length,
            py::arg("preamble_length"),
            "Set LoRa preamble length in symbols (range: 0 - 65535)")
        .def("set_crc", &SX1262Wrapper::set_crc,
            py::arg("enable"),
            "Enable (True/2) or disable (False/0) LoRa CRC checksum")
        .def("set_tcxo", &SX1262Wrapper::set_tcxo,
            py::arg("voltage"),
            py::arg("delay") = 5000,
            "Set TCXO reference voltage (1.6 - 3.3V, or 0 to disable for XTAL crystals)")
        .def("set_dio2_as_rf_switch", &SX1262Wrapper::set_dio2_as_rf_switch,
            py::arg("enable") = true,
            "Configure DIO2 pin to control RF switch automatically")

        // ---------------------------------------------------------------------
        // 5. Signal Quality & Status Queries
        // ---------------------------------------------------------------------
        .def("get_rssi", &SX1262Wrapper::get_rssi,
            "Get RSSI of the last received packet in dBm")
        .def("get_snr", &SX1262Wrapper::get_snr,
            "Get SNR of the last received packet in dB")
        .def_property_readonly("rssi", &SX1262Wrapper::get_rssi,
            "RSSI of the last received packet in dBm")
        .def_property_readonly("snr", &SX1262Wrapper::get_snr,
            "SNR of the last received packet in dB")
        .def_property_readonly("last_status", &SX1262Wrapper::get_last_status,
            "Most recent RadioLib integer status code")
        .def("get_status_text", [](SX1262Wrapper& self) {
            return SX1262Wrapper::get_status_text(self.get_last_status());
        }, "Human-readable description of the most recent status code")

        // ---------------------------------------------------------------------
        // 6. Resource Management & Context Manager
        // ---------------------------------------------------------------------
        .def("close", &SX1262Wrapper::close,
            "Release GPIO, SPI, and radio hardware resources")
        .def("__enter__", [](SX1262Wrapper& self) -> SX1262Wrapper& {
            return self;
        })
        .def("__exit__", [](SX1262Wrapper& self, py::object, py::object, py::object) {
            self.close();
        });
}
