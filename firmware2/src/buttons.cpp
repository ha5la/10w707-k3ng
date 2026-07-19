#include <Arduino.h>
#include "config.h"
#include "buttons.h"
#include "servo.h"

// Hold-to-rotate: press starts manual rotation, release stops immediately
// (the amplitude ramp in dds provides the mechanical gentleness).

static uint32_t last_ms;
static uint8_t cw_count, ccw_count;
static bool cw_held, ccw_held;

void buttons_init() {
  pinMode(PIN_BUTTON_CW, INPUT_PULLUP);
  pinMode(PIN_BUTTON_CCW, INPUT_PULLUP);
}

static bool debounce(uint8_t pin, uint8_t *count) {
  if (digitalRead(pin) == LOW) {
    if (*count < BUTTON_DEBOUNCE_TICKS) (*count)++;
  } else if (*count > 0) {
    (*count)--;
  }
  return *count >= BUTTON_DEBOUNCE_TICKS;
}

void buttons_tick(uint32_t now_ms) {
  if (now_ms - last_ms < BUTTON_TICK_MS) return;
  last_ms = now_ms;

  const bool cw = debounce(PIN_BUTTON_CW, &cw_count);
  const bool ccw = debounce(PIN_BUTTON_CCW, &ccw_count);

  if (cw && !cw_held) servo_manual_cw();
  if (ccw && !ccw_held) servo_manual_ccw();
  if ((!cw && cw_held && servo_get_state() == SERVO_MAN_CW) ||
      (!ccw && ccw_held && servo_get_state() == SERVO_MAN_CCW)) {
    servo_stop();
  }

  cw_held = cw;
  ccw_held = ccw;
}
