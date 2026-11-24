#include "sx127x_bindings.h"
#include "sx1272_wrapper.h"
#include "sx1273_wrapper.h"
#include "sx1276_wrapper.h"
#include "sx1277_wrapper.h"
#include "sx1278_wrapper.h"
#include "sx1279_wrapper.h"
#include "status_codes.h"
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <cstdio>

namespace py = pybind11;

template <typename WrapperT>
static void bind_sx127x_class(
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
            py::arg("dio0") = 25,
            py::arg("reset") = 22,
            py::arg("dio1") = -1,
            "Instantiate radio with SPI and GPIO pin assignments")
        .def("begin", &WrapperT::begin,
            py::arg("frequency") = default_freq,
            py::arg("bandwidth") = 125.0f,
            py::arg("spreading_factor") = 7,
            py::arg("coding_rate") = 5,
            py::arg("sync_word") = 0x12,
            py::arg("power") = default_power,
            py::arg("preamble_length") = 8,
            py::arg("gain") = 0,
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
            "Clean up transmitter after transmission is finished (puts radio in standby)")
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
            "Enter standby mode")
        .def("sleep", &WrapperT::sleep,
            "Enter low-power sleep mode")
        .def("reset", &WrapperT::reset,
            "Hardware reset the radio chip")

        // ---------------------------------------------------------------------
        // 4. Runtime RF & Modem Settings
        // ---------------------------------------------------------------------
        .def("set_frequency", &WrapperT::set_frequency,
            py::arg("freq"),
            "Change carrier frequency in MHz")
        .def("set_bandwidth", &WrapperT::set_bandwidth,
            py::arg("bw"),
            "Change LoRa bandwidth in kHz")
        .def("set_spreading_factor", &WrapperT::set_spreading_factor,
            py::arg("sf"),
            "Change LoRa spreading factor (6 - 12)")
        .def("set_coding_rate", &WrapperT::set_coding_rate,
            py::arg("cr"),
            "Change LoRa coding rate denominator (5 - 8 for 4/5 - 4/8)")
        .def("set_output_power", &WrapperT::set_output_power,
            py::arg("power"),
            py::arg("force_rfo") = false,
            "Change transmission output power in dBm")
        .def("set_gain", &WrapperT::set_gain,
            py::arg("gain"),
            "Set receiver LNA gain (1 - 6, or 0 for AGC)")
        .def("set_sync_word", &WrapperT::set_sync_word,
            py::arg("sync_word"),
            "Set 1-byte LoRa sync word (e.g. 0x12 for private, 0x34 for public/LoRaWAN)")
        .def("set_current_limit", &WrapperT::set_current_limit,
            py::arg("current_limit"),
            "Set over-current protection limit in mA (range: 45 - 240 mA; 0 to disable)")
        .def("set_preamble_length", &WrapperT::set_preamble_length,
            py::arg("preamble_length"),
            "Set LoRa preamble length in symbols (range: 6 - 65535)")
        .def("set_crc", &WrapperT::set_crc,
            py::arg("enable"),
            py::arg("mode") = false,
            "Enable (True) or disable (False) LoRa CRC checksum")

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

void bind_sx127x(py::module_& m) {
    // Register SX1272, SX1273, SX1276, SX1277, SX1278, SX1279 directly in root radiogem module
    bind_sx127x_class<SX1272Wrapper>(
        m,
        "SX1272",
        "RadioLib SX1272 driver for Raspberry Pi (860-1020 MHz, 2 to 17/20 dBm)",
        915.0f,
        10
    );

    bind_sx127x_class<SX1273Wrapper>(
        m,
        "SX1273",
        "RadioLib SX1273 driver for Raspberry Pi (860-1020 MHz, SF 6-9, 2 to 17/20 dBm)",
        915.0f,
        10
    );

    bind_sx127x_class<SX1276Wrapper>(
        m,
        "SX1276",
        "RadioLib SX1276 driver for Raspberry Pi (137-1020 MHz, 2 to 17/20 dBm)",
        868.0f,
        10
    );

    bind_sx127x_class<SX1277Wrapper>(
        m,
        "SX1277",
        "RadioLib SX1277 driver for Raspberry Pi (137-1020 MHz, SF 6-9, 2 to 17/20 dBm)",
        868.0f,
        10
    );

    bind_sx127x_class<SX1278Wrapper>(
        m,
        "SX1278",
        "RadioLib SX1278 driver for Raspberry Pi (137-525 MHz, 2 to 17/20 dBm)",
        434.0f,
        10
    );

    bind_sx127x_class<SX1279Wrapper>(
        m,
        "SX1279",
        "RadioLib SX1279 driver for Raspberry Pi (137-960 MHz, 2 to 17/20 dBm)",
        434.0f,
        10
    );

    // Also expose them in a dedicated 'sx127x' submodule for generation-based namespacing
    py::module_ sx127x_mod = m.def_submodule("sx127x", "Semtech 1st generation SX127x LoRa transceiver family");
    sx127x_mod.attr("SX1272") = m.attr("SX1272");
    sx127x_mod.attr("SX1273") = m.attr("SX1273");
    sx127x_mod.attr("SX1276") = m.attr("SX1276");
    sx127x_mod.attr("SX1277") = m.attr("SX1277");
    sx127x_mod.attr("SX1278") = m.attr("SX1278");
    sx127x_mod.attr("SX1279") = m.attr("SX1279");
}

