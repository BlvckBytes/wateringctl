#ifndef valve_control_h
#define valve_control_h

#include <SD.h>
#include <blvckstd/jsonh.h>

#include "shift_register.h"
#include "scheduler_time.h"
#include "sd_handler.h"
#include "web_server/sockets/web_server_socket_events.h"

// Maximum number of valves that can be attached to the system
#define VALVE_CONTROL_NUM_VALVES 8

// Maximum number of characters a valve alias string can have
#define VALVE_CONTROL_ALIAS_MAXLEN 16

// Full path of the file that persistent data will be r/w from/to
#define VALVE_CONTROL_FILE "/data/valves.bin"

typedef struct valve
{
  char alias[VALVE_CONTROL_ALIAS_MAXLEN];   // Alias name (human readable string)
  bool state;                               // Current on/off state
  bool disabled;                            // Disable state
  scheduler_time_t timer;                   // Timers remaining time
  bool has_timer;                           // Whether or not this valve has an active timer
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
 * @brief Load all valve aliases from a file
 * 
 * @param vc Valve controller handle
 */
void valve_control_file_load(valve_control_t *vc);

/**
 * @brief Store all valve aliases to a file
 * 
 * @param vc Valve controller handle
 */
void valve_control_file_save(valve_control_t *vc);

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