#include "valve_control.h"

valve_t valve_control_valve_make(const char *alias, bool disabled)
{
  valve_t res = { { 0 }, false, disabled, SCHEDULER_TIME_MIDNIGHT, false };

  // Copy over the alias into the struct with a maximum length
  strncpy(res.alias, alias, VALVE_CONTROL_ALIAS_MAXLEN);

  return res;
}

valve_control_t valve_control_make()
{
  valve_control_t vc;

  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    valve_t *valve = &(vc.valves[i]);
    *valve = valve_control_valve_make("?", false);
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

  // Broadcast valve on/off event
  scptr char *ev_arg = strfmt_direct("%d", valve_id);
  web_server_socket_events_broadcast(state ? WSE_VALVE_ON : WSE_VALVE_OFF, ev_arg);

  // Set the valve's state and apply it to the output
  vc->valves[valve_id].state = state;
  valve_control_apply_state(vc);
}

/**
 * @brief Parse a valve from it's JSON representation and apply it's values
 * 
 * @param vc Valve control instance
 * @param jsn Json to parse
 * @param index Index of the target valve
 */
INLINED void valve_control_json_parse_valve(valve_control_t *vc, htable_t *jsn, int index)
{
  // Make sure the index is valid
  if (index < 0 || index >= VALVE_CONTROL_NUM_VALVES)
    return;

  // Try to get the current valve's alias
  char *alias = NULL;
  if (jsonh_get_str(jsn, "alias", &alias) != JOPRES_SUCCESS)
    return;

  // Apply values
  valve_t *valve = &(vc->valves[index]);
  strncpy(valve->alias, alias, VALVE_CONTROL_ALIAS_MAXLEN);

  // Try to get the current valve's disabled state
  jsonh_get_bool(jsn, "disabled", &(valve->disabled));
}

void valve_control_file_load(valve_control_t *vc)
{
  // Read the json file
  scptr htable_t *jsn = sdh_read_json_file(VALVE_CONTROL_FILE);
  if (!jsn)
    return;

  // Try to get the valves
  dynarr_t *valves_jsn = NULL;
  if (jsonh_get_arr(jsn, "valves", &valves_jsn) != JOPRES_SUCCESS)
    return;

  // Loop all stored valves
  scptr size_t *active = NULL;
  size_t num_active = 0;
  dynarr_indices(valves_jsn, &active, &num_active);
  for (size_t i = 0; i < num_active; i++)
  {
    int index = active[i];

    // Try to get the current valve json
    htable_t *valve_jsn = NULL;
    if (jsonh_get_arr_obj(valves_jsn, index, &valve_jsn) != JOPRES_SUCCESS)
      return;

    valve_control_json_parse_valve(vc, valve_jsn, index);
  }
}

void valve_control_file_save(valve_control_t *vc)
{
  scptr htable_t *jsn = htable_make(1, mman_dealloc_nr);
  scptr dynarr_t *valves_jsn = dynarr_make_mmf(VALVE_CONTROL_NUM_VALVES);
  jsonh_set_arr_ref(jsn, "valves", valves_jsn);

  // Loop all valves and add them to the array
  for (int i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    scptr htable_t *valve_jsn = valve_control_valve_jsonify(vc, i);
    jsonh_insert_arr_obj_ref(valves_jsn, valve_jsn);
  }

  // Write json to the file
  if (!sdh_write_json_file(jsn, VALVE_CONTROL_FILE))
  {
    dbgerr("Could not write the valves file");
    return;
  }
}

htable_t *valve_control_valve_jsonify(valve_control_t *vc, size_t valve_id)
{
  // Index out of range
  if (valve_id >= VALVE_CONTROL_NUM_VALVES)
    return NULL;

  // Create a new node
  scptr htable_t *res = htable_make(5, mman_dealloc_nr);
  valve_t valve = vc->valves[valve_id];

  // Set alias string
  scptr char *alias = strclone_s(valve.alias, VALVE_CONTROL_ALIAS_MAXLEN);
  if (jsonh_set_str(res, "alias", (char *) mman_ref(alias)) != JOPRES_SUCCESS)
    mman_dealloc(alias);

  // Set timer string
  scptr char *timer = scheduler_time_stringify(&(valve.timer));
  if (jsonh_set_str(res, "timer", (char *) mman_ref(timer)) != JOPRES_SUCCESS)
    mman_dealloc(timer);

  jsonh_set_bool(res, "state", valve.state);
  jsonh_set_bool(res, "disabled", valve.disabled);
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

  // Get the disabled state
  bool disabled_b = NULL;
  if ((jopr = jsonh_get_bool(json, "disabled", &disabled_b)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr("disabled", jopr);
    return false;
  }

  *out = valve_control_valve_make(alias_s, disabled_b);
  return true;
}