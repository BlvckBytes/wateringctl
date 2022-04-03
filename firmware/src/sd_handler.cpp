#include <sd_handler.h>

// Link against the SD-lib's "hidden" (non-header-entry) sdCommand routine, used for hotplug

/*
============================================================================
                              Basic SD control                              
============================================================================
*/

static bool sdh_avail = false;

bool sdh_init()
{
  pinMode(SDH_PIN_INSERTED, INPUT);

  // Begin SD library using known pin layout
  if (SD.begin(SDH_PIN_CS, SPI, SDH_SPI_FREQ)) {
    dbginf("SD card slot initialized (type=%" PRIu8 ")", SD.cardType());
    sdh_avail = true;
  } else {
    dbgerr("Could not initialize SD card slot");
    sdh_avail = false;
  }

  return sdh_avail;
}

bool sdh_io_available()
{
  return sdh_avail;
}

uint32_t sdh_get_total_size_mb()
{
  if (!sdh_avail) return 0;
  return sdh_bytes_to_mb(SD.totalBytes());
}

static long sdh_last_hotplug_watch = millis();

void sdh_watch_hotplug()
{
  // Watch interval timer
  if (millis() - sdh_last_hotplug_watch < SDH_HOTPLUG_WATCH_DEL) return;
  sdh_last_hotplug_watch = millis();

  // Try to init SD now
  if (!sdh_avail)
  {
    // Still not accessible, try again at next iteration
    if (!sdh_init())
      return;
  }

  // Pin is high, thus not pulled down to GND by the slot switch
  if (digitalRead(SDH_PIN_INSERTED) == 1)
  {
    sdh_avail = false;
    SD.end();
    dbginf("Shut down SD card slot");
    return;
  }
}