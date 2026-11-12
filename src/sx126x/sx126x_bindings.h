#ifndef RADIOGEM_SX126X_BINDINGS_H
#define RADIOGEM_SX126X_BINDINGS_H

#include <pybind11/pybind11.h>

/**
 * @brief Bind 2nd generation Semtech SX126x family (SX1261, SX1262, SX1268) to pybind11 module.
 */
void bind_sx126x(pybind11::module_& m);

#endif // RADIOGEM_SX126X_BINDINGS_H

