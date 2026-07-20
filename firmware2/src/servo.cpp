#include <Arduino.h>
#include "config.h"
#include "servo.h"
#include "position.h"
#include "dds.h"

static servo_state state = SERVO_IDLE;
static int16_t target_deg10;
static uint32_t last_tick_ms;

servo_state servo_get_state() { return state; }
int16_t servo_get_target_deg10() { return target_deg10; }

void servo_goto(int16_t t) {
  if (t < 0) t = 0;
  if (t > 3600) t = 3600;
  target_deg10 = t;
  state = SERVO_GOTO;
}

void servo_manual_cw()  { state = SERVO_MAN_CW; }
void servo_manual_ccw() { state = SERVO_MAN_CCW; }

void servo_stop() {
  state = SERVO_IDLE;
  dds_stop();
}

// Full speed far from target, tapering linearly to crawl inside SERVO_TAPER.
static uint16_t profile_freq(int16_t abs_err) {
  if (abs_err >= SERVO_TAPER_DEG10) return DRIVE_FREQ_MAX_DHZ;
  return DRIVE_FREQ_MIN_DHZ +
         (uint16_t)(((uint32_t)abs_err *
                     (DRIVE_FREQ_MAX_DHZ - DRIVE_FREQ_MIN_DHZ)) /
                    SERVO_TAPER_DEG10);
}

void servo_tick(uint32_t now_ms) {
  if (now_ms - last_tick_ms < SERVO_TICK_MS) return;
  last_tick_ms = now_ms;

  const int16_t pos = position_deg10();

  switch (state) {
    case SERVO_IDLE:
      break;

    case SERVO_GOTO: {
      const int16_t err = target_deg10 - pos;
      const int16_t abs_err = err < 0 ? -err : err;
      if (abs_err <= SERVO_DEADBAND_DEG10) {
        servo_stop();
      } else {
        dds_run(profile_freq(abs_err), err > 0 ? DDS_CW : DDS_CCW);
      }
      break;
    }

    case SERVO_MAN_CW:
      if (pos >= 3600 - SERVO_SOFT_LIMIT_DEG10) servo_stop();
      else dds_run(DRIVE_FREQ_MAX_DHZ, DDS_CW);
      break;

    case SERVO_MAN_CCW:
      if (pos <= SERVO_SOFT_LIMIT_DEG10) servo_stop();
      else dds_run(DRIVE_FREQ_MAX_DHZ, DDS_CCW);
      break;
  }
}
