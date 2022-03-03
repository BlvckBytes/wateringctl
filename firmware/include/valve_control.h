#ifndef valve_control_h
#define valve_control_h

#include "scheduler.h"

// Maximum number of valves that can be attached to the system
#define VALVE_CONTROL_NUM_VALVES 8

// Maximum number of characters a valve alias string can have
#define VALVE_CONTROL_ALIAS_MAXLEN 16

// Number of bytes valve control needs to persist it's aliases in the EEPROM
#define VALVE_CONTROL_EEPROM_FOOTPRINT (VALVE_CONTROL_NUM_VALVES * VALVE_CONTROL_ALIAS_MAXLEN)

typedef struct valve
{
  char alias[VALVE_CONTROL_ALIAS_MAXLEN];   // Alias name (human readable string)
  bool state;                               // Current on/off state
} valve_t;

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

#endif