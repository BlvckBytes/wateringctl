#include "shift_register.h"

void shift_register_init()
{
  // Set all involved pins as outputs
  pinMode(SHIFT_REGISTER_DATA, OUTPUT);
  pinMode(SHIFT_REGISTER_SHIFT, OUTPUT);
  pinMode(SHIFT_REGISTER_STORE, OUTPUT);
}

void shift_register_set_bits(uint64_t bits)
{
  // Disable store line
  digitalWrite(SHIFT_REGISTER_STORE, LOW);

  // Shift out bits
  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    digitalWrite(SHIFT_REGISTER_DATA, (bits >> (VALVE_CONTROL_NUM_VALVES - 1 - i)) & 0x1);
    delay(1);
    digitalWrite(SHIFT_REGISTER_SHIFT, HIGH);
    delay(1);
    digitalWrite(SHIFT_REGISTER_SHIFT, LOW);
  }

  // Store the shifted bits into the output-registers on the rising edge
  delay(1);
  digitalWrite(SHIFT_REGISTER_STORE, HIGH);
  delay(1);
  digitalWrite(SHIFT_REGISTER_STORE, LOW);
}

void shift_register_clear()
{
  // Clear out all bits
  shift_register_set_bits(0);
}