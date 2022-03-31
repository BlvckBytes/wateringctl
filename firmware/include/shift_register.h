#ifndef shift_register_h
#define shift_register_h

#include <inttypes.h>
#include <Arduino.h>

#include "valve_control.h"

#define SHIFT_REGISTER_DATA  25     // Shift register data pin
#define SHIFT_REGISTER_STORE 33     // Shift register store pulse pin
#define SHIFT_REGISTER_SHIFT 32     // Shift register shift pulse pin

/**
 * @brief Initialize the shift-register pins
 */
void shift_register_init();

/**
 * @brief Clear all bits within the shift register
 */
void shift_register_clear();

/**
 * @brief Set all available bits at once
 * 
 * @param bits Bits, where the LSB is mapped to output Q0
 */
void shift_register_set_bits(uint64_t bits);

#endif