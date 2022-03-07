#ifndef valve_control_h
#define valve_control_h

#include <blvckstd/jsonh.h>

#include "scheduler.h"
#include "shift_register.h"

// Maximum number of valves that can be attached to the system
#define VALVE_CONTROL_NUM_VALVES 8

// Maximum number of characters a valve alias string can have
#define VALVE_CONTROL_ALIAS_MAXLEN 16

// Disabled states, bytepacked
#define VALVE_CONTROL_NUM_DISABLED_BYTES ((VALVE_CONTROL_NUM_VALVES + 7) / 8)

// Number of bytes valve control needs to persist it's aliases in the EEPROM
#define VALVE_CONTROL_EEPROM_FOOTPRINT (                                   \
  VALVE_CONTROL_NUM_VALVES * VALVE_CONTROL_ALIAS_MAXLEN /* Aliases */      \
  + VALVE_CONTROL_NUM_DISABLED_BYTES /* Disabled bytes */                  \
)

typedef struct valve
{
  char alias[VALVE_CONTROL_ALIAS_MAXLEN];   // Alias name (human readable string)
  bool state;                               // Current on/off state
  bool disabled;                            // Disable state
} valve_t;

/**
 * @brief Create a valve struct by it's properties
 * 
 * @param alias Alias string, will get capped off to the max. length automatically
 * @param disabled Disabled state of this valve
 */
valve_t valve_control_valve_make(const char *alias, bool disabled);

typedef struct valve_control
{
  valve_t valves[VALVE_CONTROL_NUM_VALVES]; // Valve table
} valve_control_t;

valve_control_t valve_control_make();

/**
 * @brief Load all valve aliases from EEPROM
 * 
 * @param vc Valve controller handle
 */
void valve_control_eeprom_load(valve_control_t *vc);

/**
 * @brief Store all valve aliases to EEPROM
 * 
 * @param vc Valve controller handle
 */
void valve_control_eeprom_save(valve_control_t *vc);

/**
 * @brief Toggle a specific valve's state
 * 
 * @param vc Valve controller handle
 * @param valve_id ID of the target valve
 * @param state New valve state
 */
void valve_control_toggle(valve_control_t *vc, size_t valve_id, bool state);

/**
 * @brief Transform a valve into it's JSONH array data-structure
 * 
 * @param vc Valve controller handle
 * @param valve_id ID of the target valve
 * 
 * @return htable_t* JSONH data structure
 */
htable_t *valve_control_valve_jsonify(valve_control_t *vc, size_t valve_id);

/**
 * @brief Parsse a valve's writable values from json, using the following schema:
 * 
 * {
 *   "alias": "...",
 *   "disabled": <boolean>
 * }
 * 
 * @param json JSONH json node
 * @param err Error output buffer
 * @param out Value output buffer
 * 
 * @return true Parsing successful
 * @return false Parsing error, see err
 */
bool valve_control_valve_parse(htable_t *json, char **err, valve_t *out);

#endif