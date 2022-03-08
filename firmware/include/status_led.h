#ifndef status_led_h
#define status_led_h

#include <Arduino.h>
#include <blvckstd/enumlut.h>

#define STATUS_LED_PIN 13

#define _EVALS_STATUS_LED(FUN) \
  FUN(STATLED_ERROR, 0x0)      \
  FUN(STATLED_CONNECTING, 0x1) \
  FUN(STATLED_CONNECTED, 0x2)

ENUM_TYPEDEF_FULL_IMPL(status_led, _EVALS_STATUS_LED);

/**
 * @brief Initialize the output pin and start out in connecting mode
 */
void status_led_init();

/**
 * @brief Update the led thus blink according to the current state
 */
void status_led_update();

/**
 * @brief Set the current status displayed by the status led
 * 
 * @param status Status
 */
void status_led_set(status_led_t status);

#endif