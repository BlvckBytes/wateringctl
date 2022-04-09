#include <sd_handler.h>

// Link against the SD-lib's "hidden" (non-header-entry) sdCommand routine, used for hotplug

/*
============================================================================
                              Basic SD control                              
============================================================================
*/

static bool sdh_avail = false;

INLINED static void sdh_check_file_existence()
{
  // TODO: Implement creating std-files like www/index.html or data as well as log files
}

bool sdh_init()
{
  pinMode(SDH_PIN_INSERTED, INPUT);

  // Begin SD library using known pin layout
  if (SD.begin(SDH_PIN_CS, SPI, SDH_SPI_FREQ)) {
    sdh_check_file_existence();
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

/*
============================================================================
                              R/W Utilities                                
============================================================================
*/

File sdh_open_write_ensure_parent_dirs(const char *path)
{
  scptr char *ppath = strclone(path);

  // Start at index 1 to skip the root slash
  int last_index = 1;

  while (true)
  {
    // Get the next / from (including) the last index
    char *curr_c = strchr(&ppath[last_index], '/');

    // No more slashes in the string
    if (!curr_c)
      break;

    // Buffer the char and temporarily terminate ppath here
    char buf = *curr_c;
    *curr_c = 0;

    // Make the directory if it doesn't yet exist
    if (!SD.exists(ppath))
      SD.mkdir(ppath);

    // Restore ppath's state
    *curr_c = buf;

    // Search after the current occurrence next iteration
    last_index = curr_c - ppath + 1;
  }

  // Parent dirs should now exist, try to open the file for writing
  return SD.open(path, "w");
}