#include "status_led.h"

ENUM_LUT_FULL_IMPL(status_led, _EVALS_STATUS_LED);

static status_led_t current_status;   // Currently displayed status
static uint32_t last_pulse;           // Last millis() stamp when the LED pulsed
static bool led_state;                // Current LED lighting state

// Status to pulse duration LUT
static uint32_t pulse_durations[] = {
  [STATLED_ERROR] = 100,
  [STATLED_CONNECTING] = 300,
  [STATLED_CONNECTED] = 2000
};

void status_led_init()
{
  // Initialize pin
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Start out in connecting mode
  current_status = STATLED_CONNECTING;
  last_pulse = 0;
  led_state = false;
}

void status_led_update()
{
  // Status not yet registered
  if (current_status >= (sizeof(pulse_durations) / sizeof(long)))
    return;

  // Delay by skipping invocations
  uint32_t delay = pulse_durations[current_status];
  if (millis() < last_pulse + delay)
    return;

  // Toggle LED
  led_state = !led_state;
  digitalWrite(STATUS_LED_PIN, led_state);
  last_pulse = millis();
}

void status_led_set(status_led_t status)
{
  current_status = status;
}