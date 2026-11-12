#include "sx126x_bindings.h"
#include "sx1261_wrapper.h"
#include "sx1262_wrapper.h"
#include "sx1268_wrapper.h"
#include "status_codes.h"
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <cstdio>

namespace py = pybind11;

template <typename WrapperT>
static void bind_sx126x_class(
    py::module_& m,
    const char* class_name,
    const char* doc_str,
    float default_freq,
    int8_t default_power
) {
    py::class_<WrapperT>(m, class_name, doc_str)
        .def(py::init<uint8_t, uint32_t, uint8_t, uint8_t, int32_t, int32_t, int32_t, int32_t>(),
            py::arg("spi_channel") = 1,
            py::arg("spi_speed") = 2000000,
            py::arg("spi_device") = 0,
            py::arg("gpio_device") = 0,
            py::arg("nss") = 7,
            py::arg("dio1") = 27,
            py::arg("reset") = 22,
            py::arg("busy") = 24,
            "Instantiate radio with SPI and GPIO pin assignments")
        .def("begin", &WrapperT::begin,
            py::arg("frequency") = default_freq,
            py::arg("bandwidth") = 125.0f,
            py::arg("spreading_factor") = 7,
            py::arg("coding_rate") = 5,
            py::arg("sync_word") = 0x12,
            py::arg("power") = default_power,
            py::arg("preamble_length") = 8,
            py::arg("tcxo_voltage") = 1.6f,
            py::arg("use_regulator_ldo") = false,
            "Initialize radio modem for LoRa. Returns status code (0 = success)")

        // ---------------------------------------------------------------------
        // 1. Blocking Transmit & Receive
        // ---------------------------------------------------------------------
        .def("transmit", [](WrapperT& self, const py::object& data) {
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
        .def("receive", &WrapperT::receive,
            py::call_guard<py::gil_scoped_release>(),
            py::arg("timeout_ms") = 0,
            py::arg("return_none_on_error") = true,
            "Receive a LoRa packet (blocking with timeout). Releases GIL. Returns Packet or None")

        // ---------------------------------------------------------------------
        // 2. Non-Blocking / Interrupt Transmit & Receive
        // ---------------------------------------------------------------------
        .def("start_transmit", [](WrapperT& self, const py::object& data) {
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
        .def("finish_transmit", &WrapperT::finish_transmit,
            "Clean up transmitter after transmission is finished (powers down RF switch, puts radio in standby)")
        .def("start_receive", &WrapperT::start_receive,
            py::arg("timeout_ms") = 0,
            "Start non-blocking packet reception (0 = continuous listen mode until packet arrives). Returns status code (0 = success)")
        .def("read_data", &WrapperT::read_data,
            py::arg("return_none_on_error") = true,
            "Read packet data after receiving an interrupt alert. Returns Packet or None")
        .def("set_packet_received_action", [](WrapperT& self, py::function func) {
            self.set_packet_received_action([func]() {
                py::gil_scoped_acquire acquire;
                try {
                    func();
                } catch (const py::error_already_set& e) {
                    fprintf(stderr, "Exception in Python packet_received callback: %s\n", e.what());
                }
            });
        }, py::arg("callback"), "Set a Python callback function to be called when a complete packet is received via interrupt")
        .def("clear_packet_received_action", &WrapperT::clear_packet_received_action,
            "Clear the packet received callback")
        .def("set_packet_sent_action", [](WrapperT& self, py::function func) {
            self.set_packet_sent_action([func]() {
                py::gil_scoped_acquire acquire;
                try {
                    func();
                } catch (const py::error_already_set& e) {
                    fprintf(stderr, "Exception in Python packet_sent callback: %s\n", e.what());
                }
            });
        }, py::arg("callback"), "Set a Python callback function to be called when packet transmission finishes via interrupt")
        .def("clear_packet_sent_action", &WrapperT::clear_packet_sent_action,
            "Clear the packet sent callback")
        .def_property_readonly("has_received", &WrapperT::has_received,
            "True if a packet has been received via interrupt since last read")
        .def_property_readonly("has_sent", &WrapperT::has_sent,
            "True if packet transmission finished via interrupt")
        .def("clear_flags", &WrapperT::clear_flags,
            "Reset internal interrupt status flags")

        // ---------------------------------------------------------------------
        // 3. Power & Mode Management
        // ---------------------------------------------------------------------
        .def("standby", &WrapperT::standby,
            py::arg("mode") = 1,
            "Enter standby mode (1 = STDBY_RC, 2 = STDBY_XOSC)")
        .def("sleep", &WrapperT::sleep,
            py::arg("retain_config") = false,
            "Enter low-power sleep mode")

        // ---------------------------------------------------------------------
        // 4. Runtime RF & Modem Settings
        // ---------------------------------------------------------------------
        .def("set_frequency", &WrapperT::set_frequency,
            py::arg("freq"),
            "Change carrier frequency in MHz (e.g. 866.5)")
        .def("set_bandwidth", &WrapperT::set_bandwidth,
            py::arg("bw"),
            "Change LoRa bandwidth in kHz (e.g. 125.0, 250.0, 500.0)")
        .def("set_spreading_factor", &WrapperT::set_spreading_factor,
            py::arg("sf"),
            "Change LoRa spreading factor (5 - 12)")
        .def("set_coding_rate", &WrapperT::set_coding_rate,
            py::arg("cr"),
            "Change LoRa coding rate denominator (5 - 8 for 4/5 - 4/8)")
        .def("set_output_power", &WrapperT::set_output_power,
            py::arg("power"),
            "Change transmission output power in dBm")
        .def("set_sync_word", &WrapperT::set_sync_word,
            py::arg("sync_word"),
            py::arg("control_bits") = 0x44,
            "Set 1-byte LoRa sync word (e.g. 0x12 for private, 0x34 for public/LoRaWAN)")
        .def("set_current_limit", &WrapperT::set_current_limit,
            py::arg("current_limit"),
            "Set over-current protection limit in mA (0 to disable)")
        .def("set_preamble_length", &WrapperT::set_preamble_length,
            py::arg("preamble_length"),
            "Set LoRa preamble length in symbols (range: 0 - 65535)")
        .def("set_crc", &WrapperT::set_crc,
            py::arg("enable"),
            "Enable (True/2) or disable (False/0) LoRa CRC checksum")
        .def("set_tcxo", &WrapperT::set_tcxo,
            py::arg("voltage"),
            py::arg("delay") = 5000,
            "Set TCXO reference voltage (1.6 - 3.3V, or 0 to disable for XTAL crystals)")
        .def("set_dio2_as_rf_switch", &WrapperT::set_dio2_as_rf_switch,
            py::arg("enable") = true,
            "Configure DIO2 pin to control RF switch automatically")

        // ---------------------------------------------------------------------
        // 5. Signal Quality & Status Queries
        // ---------------------------------------------------------------------
        .def("get_rssi", &WrapperT::get_rssi,
            "Get RSSI of the last received packet in dBm")
        .def("get_snr", &WrapperT::get_snr,
            "Get SNR of the last received packet in dB")
        .def_property_readonly("rssi", &WrapperT::get_rssi,
            "RSSI of the last received packet in dBm")
        .def_property_readonly("snr", &WrapperT::get_snr,
            "SNR of the last received packet in dB")
        .def_property_readonly("last_status", &WrapperT::get_last_status,
            "Most recent RadioLib integer status code")
        .def("get_status_text", [](WrapperT& self) {
            return get_status_text(self.get_last_status());
        }, "Human-readable description of the most recent status code")

        // ---------------------------------------------------------------------
        // 6. Resource Management & Context Manager
        // ---------------------------------------------------------------------
        .def("close", &WrapperT::close,
            "Release GPIO, SPI, and radio hardware resources")
        .def("__enter__", [](WrapperT& self) -> WrapperT& {
            return self;
        })
        .def("__exit__", [](WrapperT& self, py::object, py::object, py::object) {
            self.close();
        });
}

void bind_sx126x(py::module_& m) {
    // Register SX1261, SX1262, SX1268 directly in root radiogem module
    bind_sx126x_class<SX1261Wrapper>(
        m,
        "SX1261",
        "RadioLib SX1261 driver for Raspberry Pi (-17 to +15 dBm)",
        866.5f,
        10
    );

    bind_sx126x_class<SX1262Wrapper>(
        m,
        "SX1262",
        "RadioLib SX1262 driver for Raspberry Pi (-9 to +22 dBm)",
        866.5f,
        10
    );

    bind_sx126x_class<SX1268Wrapper>(
        m,
        "SX1268",
        "RadioLib SX1268 driver for Raspberry Pi (410-810 MHz, -9 to +22 dBm)",
        434.0f,
        10
    );

    // Also expose them in a dedicated 'sx126x' submodule for generation-based namespacing
    py::module_ sx126x_mod = m.def_submodule("sx126x", "Semtech 2nd generation SX126x LoRa transceiver family");
    sx126x_mod.attr("SX1261") = m.attr("SX1261");
    sx126x_mod.attr("SX1262") = m.attr("SX1262");
    sx126x_mod.attr("SX1268") = m.attr("SX1268");
}

