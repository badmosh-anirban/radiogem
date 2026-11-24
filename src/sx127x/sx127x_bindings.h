#ifndef RADIOGEM_SX127X_BINDINGS_H
#define RADIOGEM_SX127X_BINDINGS_H

#include <pybind11/pybind11.h>

/**
 * @brief Bind 1st generation Semtech SX127x family (SX1272, SX1273, SX1276, SX1277, SX1278, SX1279) to pybind11 module.
 */
void bind_sx127x(pybind11::module_& m);

#endif // RADIOGEM_SX127X_BINDINGS_H

