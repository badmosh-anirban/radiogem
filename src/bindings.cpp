#include <pybind11/pybind11.h>
#include "common/status_codes.h"
#include "sx126x/sx126x_bindings.h"

namespace py = pybind11;

PYBIND11_MODULE(radiogem, m) {
    m.doc() = "Python bindings for RadioLib LoRa radios on Raspberry Pi and embedded Linux";

    // 1. RadioLib Status Codes and text helper
    bind_status_codes(m);

    // 2. Common Packet struct
    bind_packet(m);

    // 3. Semtech 2nd generation: SX126x (SX1261, SX1262, SX1268)
    bind_sx126x(m);
}
