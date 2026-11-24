#include "status_codes.h"
#include "packet.h"
#include <RadioLib.h>
#include <pybind11/stl.h>

namespace py = pybind11;

std::string get_status_text(int16_t status) {
    switch (status) {
        case RADIOLIB_ERR_NONE:
            return "SUCCESS (0): Method executed successfully";
        case RADIOLIB_ERR_UNKNOWN:
            return "UNKNOWN_ERROR (-1): Unexpected error occurred";
        case RADIOLIB_ERR_CHIP_NOT_FOUND:
            return "CHIP_NOT_FOUND (-2): Radio chip not responding; check SPI wiring, channel, and NSS pin";
        case RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED:
            return "MEMORY_ALLOCATION_FAILED (-3): Buffer memory allocation failed";
        case RADIOLIB_ERR_PACKET_TOO_LONG:
            return "PACKET_TOO_LONG (-4): Packet length exceeds maximum supported by radio";
        case RADIOLIB_ERR_TX_TIMEOUT:
            return "TX_TIMEOUT (-5): Transmit operation timed out";
        case RADIOLIB_ERR_RX_TIMEOUT:
            return "RX_TIMEOUT (-6): Timed out waiting for incoming packet";
        case RADIOLIB_ERR_CRC_MISMATCH:
            return "CRC_MISMATCH (-7): Received packet corrupted (CRC check failed)";
        case RADIOLIB_ERR_INVALID_BANDWIDTH:
            return "INVALID_BANDWIDTH (-8): Specified bandwidth is invalid for radio";
        case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
            return "INVALID_SPREADING_FACTOR (-9): Specified spreading factor is invalid for radio";
        case RADIOLIB_ERR_INVALID_CODING_RATE:
            return "INVALID_CODING_RATE (-10): Specified coding rate is invalid for radio";
        case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
            return "INVALID_OUTPUT_POWER (-13): Specified output power is out of valid range";
        case RADIOLIB_ERR_INVALID_CRC_CONFIGURATION:
            return "INVALID_CRC_CONFIGURATION (-14): Invalid CRC configuration";
        case RADIOLIB_ERR_INVALID_CURRENT_LIMIT:
            return "INVALID_CURRENT_LIMIT (-17): Over-current protection limit out of valid range";
        case RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH:
            return "INVALID_PREAMBLE_LENGTH (-18): Specified preamble length is invalid";
        case RADIOLIB_ERR_INVALID_TCXO_VOLTAGE:
            return "INVALID_TCXO_VOLTAGE (-705): Invalid TCXO voltage (valid: 1.6, 1.7, 1.8, 2.2, 2.4, 2.7, 3.0, 3.3 V)";
        case RADIOLIB_ERR_SPI_CMD_TIMEOUT:
            return "SPI_CMD_TIMEOUT (-706): SPI command timed out (check TCXO vs XTAL setting, or BUSY pin)";
        case RADIOLIB_ERR_SPI_CMD_INVALID:
            return "SPI_CMD_INVALID (-707): SPI command invalid (check TCXO vs XTAL setting)";
        case RADIOLIB_ERR_SPI_CMD_FAILED:
            return "SPI_CMD_FAILED (-708): SPI command failed";
        default:
            return "ERROR (" + std::to_string(status) + ")";
    }
}

void bind_status_codes(py::module_& m) {
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

    m.def("get_status_text", &get_status_text,
          py::arg("status"),
          "Convert a RadioLib integer status code to human-readable text");
}

void bind_packet(py::module_& m) {
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
                       get_status_text(p.status) + ")>";
            }
        });
}

