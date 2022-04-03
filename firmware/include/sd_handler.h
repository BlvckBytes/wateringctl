#ifndef sd_handler_h
#define sd_handler_h

#include <blvckstd/dbglog.h>
#include <sd_diskio.h>
#include <inttypes.h>
#include <SPI.h>
#include <SD.h>

// SD card hardware config
#define SDH_PIN_CS 5
#define SDH_PIN_INSERTED 4
#define SDH_HOTPLUG_WATCH_DEL 500
#define SDH_SPI_FREQ 1000000UL * 8

// Drive ID, 0 should be totally fine as there never
// will be multiple SD cards in this system
#define SDH_PDRV 0

// Conversion utility
#define sdh_bytes_to_mb(bytes) (bytes / 1000 / 1000)

/**
 * @brief Initialize the SD card slot
 * 
 * @return true SD initialized
 * @return false No SD card found
 */
bool sdh_init();

/**
 * @brief Checks whether or not card I/O is available (connected and inited)
 * 
 * @return true Card is usable
 * @return false Card is unusable
 */
bool sdh_io_available();

/**
 * @brief Get the total size of the currently in-use SD card in megabytes
 * 
 * @return uint8_t Size in megabytes
 */
uint32_t sdh_get_total_size_mb();

/**
 * @brief Watches for hotplug events and updates the system accordingly
 */
void sdh_watch_hotplug();

#endif