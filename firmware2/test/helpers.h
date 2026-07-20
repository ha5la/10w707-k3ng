#pragma once
// Shared helpers for the native test suites.
#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "dds.h"
#include "servo.h"
#include "buttons.h"
#include "position.h"
#include "gs232.h"

// Advance the fake clock 1 ms at a time, running every module tick.
inline void run_ms(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i++) {
    mock_millis_now++;
    buttons_tick(mock_millis_now);
    servo_tick(mock_millis_now);
    dds_tick(mock_millis_now);
  }
}

// Advance only the dds tick (for dds-only tests).
inline void run_dds_ms(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i++) {
    mock_millis_now++;
    dds_tick(mock_millis_now);
  }
}

inline void press_cw()    { mock_pin_low[PIN_BUTTON_CW] = 1; }
inline void release_cw()  { mock_pin_low[PIN_BUTTON_CW] = 0; }
inline void press_ccw()   { mock_pin_low[PIN_BUTTON_CCW] = 1; }
inline void release_ccw() { mock_pin_low[PIN_BUTTON_CCW] = 0; }

// Analog values chosen for the default calibration (R_ccw=106, R_cw=896,
// R_fixed=820): raw12 = 4*analog, R = 820*raw/(4092-raw).
#define ANALOG_AT_CCW  117  // R ~ 106 ohm  -> ~0.0 deg
#define ANALOG_AT_MID  388  // R ~ 501 ohm  -> ~180.0 deg
#define ANALOG_AT_3550 531  // R ~ 885 ohm  -> ~355.0 deg
#define ANALOG_AT_CW   534  // R ~ 895 ohm  -> ~359.5 deg
