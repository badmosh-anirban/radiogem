#include "common/status_codes.h"
#include "sx126x/sx126x_bindings.h"
#include "sx127x/sx127x_bindings.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(radiogem, m) {
  m.doc() = "Python bindings for RadioLib LoRa radios on Raspberry Pi and "
            "embedded Linux";

  // 1. RadioLib Status Codes and text helper
  bind_status_codes(m);

  // 2. Common Packet struct
  bind_packet(m);

  // 3. Semtech 1st generation: SX127x (SX1272, SX1273, SX1276, SX1277, SX1278,
  // SX1279)
  bind_sx127x(m);

  // 4. Semtech 2nd generation: SX126x (SX1261, SX1262, SX1268)
  bind_sx126x(m);
}
