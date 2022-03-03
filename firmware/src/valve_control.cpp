#include "valve_control.h"

valve_t valve_control_valve_make(const char *alias)
{
  valve_t res = { { 0 }, false };

  // Copy over the alias into the struct with a maximum length
  strncpy(res.alias, alias, VALVE_CONTROL_ALIAS_MAXLEN);

  return res;
}

valve_control_t valve_control_make()
{
  valve_control_t vc;

  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
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
  // Enable all bits that correspond to active valves within the state number
  uint64_t state = 0;
  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
    state |= (vc->valves[i].state) << i;

  // Set the bits according to the current state
  shift_register_set_bits(state);
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

  EEPROM.commit();
}

htable_t *valve_control_valve_jsonify(valve_control_t *vc, size_t valve_id)
{
  // Index out of range
  if (valve_id >= VALVE_CONTROL_NUM_VALVES)
    return NULL;

  // Create a new node
  scptr htable_t *res = jsonh_make();
  valve_t valve = vc->valves[valve_id];

  // Set alias string
  scptr char *alias = strclone_s(valve.alias, VALVE_CONTROL_ALIAS_MAXLEN);
  if (jsonh_set_str(res, "alias", (char *) mman_ref(alias)) != JOPRES_SUCCESS)
    mman_dealloc(alias);

  // Set state boolean and identifier
  jsonh_set_bool(res, "state", valve.state);
  jsonh_set_int(res, "identifier", valve_id);

  return (htable_t *) mman_ref(res);
}

bool valve_control_valve_parse(htable_t *json, char **err, valve_t *out)
{
  // Get the alias string
  jsonh_opres_t jopr;
  char *alias_s = NULL;
  if ((jopr = jsonh_get_str(json, "alias", &alias_s)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr("alias", jopr);
    return false;
  }

  // Check for empty aliases
  size_t al = strlen(alias_s);
  if (al == 0)
  {
    *err = strfmt_direct("Empty aliases are not allowed");
    return false;
  }

  // Check for aliases that are too long to store
  if (al >= VALVE_CONTROL_ALIAS_MAXLEN)
  {
    *err = strfmt_direct("The alias has to have a length between 1 and %d characters", VALVE_CONTROL_ALIAS_MAXLEN);
    return false;
  }

  *out = valve_control_valve_make(alias_s);
  return true;
}