#ifndef RADIOGEM_STATUS_CODES_H
#define RADIOGEM_STATUS_CODES_H

#include <cstdint>
#include <string>
#include <pybind11/pybind11.h>

/**
 * @brief Convert a RadioLib integer status code to human-readable text.
 */
std::string get_status_text(int16_t status);

/**
 * @brief Bind RadioLib error constants and get_status_text function to pybind11 module.
 */
void bind_status_codes(pybind11::module_& m);

/**
 * @brief Bind common Packet struct to pybind11 module.
 */
void bind_packet(pybind11::module_& m);

#endif // RADIOGEM_STATUS_CODES_H

