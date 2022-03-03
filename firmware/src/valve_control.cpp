#include "valve_control.h"

valve_control_t valve_control_make()
{
  valve_control_t vc;

  for (int i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    // The identifier becomes it's array index, the state is initially off
    valve_t *valve = &(vc.valves[i]);
    valve->state = false;

    // Clear all alias bytes (empty string)
    for (int j = 0; j < VALVE_CONTROL_ALIAS_MAXLEN; j++)
      valve->alias[j] = 0;
  }

  return vc;
}

INLINED static void valve_control_apply_state(valve_control_t *vc)
{
  // TODO: Implement applying the current state to some outputs
}

void valve_control_toggle(valve_control_t *vc, size_t valve_id, bool state)
{
  // Valve id out of range
  if (valve_id >= VALVE_CONTROL_NUM_VALVES)
    return;

  // Set the valve's state and apply it to the output
  vc->valves[valve_id].state = state;
  valve_control_apply_state(vc);
}

void valve_control_eeprom_load(valve_control_t *vc)
{
  // Keep track of the current EEPROM address
  // Valve control comes right after the scheduler in memory
  int addr_ind = SCHEDULER_EEPROM_FOOTPRINT;

  // Loop all valves
  for (int i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    // Read the full alias
    for (int j = 0; j < VALVE_CONTROL_ALIAS_MAXLEN; j++)
    {
      // Read a char of the alias and store it into memory
      vc->valves[i].alias[j] = (char) EEPROM.read(addr_ind++);
    }
  }
}

void valve_control_eeprom_save(valve_control_t *vc)
{
  // Keep track of the current EEPROM address
  // Valve control comes right after the scheduler in memory
  int addr_ind = SCHEDULER_EEPROM_FOOTPRINT;

  // Loop all valves
  for (int i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    // Write the full alias
    for (int j = 0; j < VALVE_CONTROL_ALIAS_MAXLEN; j++)
      EEPROM.write(addr_ind++, vc->valves[i].alias[j]);
  }
}