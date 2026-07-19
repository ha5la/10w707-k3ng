#include <Arduino.h>
#include "config.h"
#include "gs232.h"
#include "position.h"
#include "servo.h"

// GS-232A subset — enough for hamlib rotctld model 603 plus manual calibration.
//
//   C          report azimuth as "+0aaa"
//   Maaa       rotate to azimuth aaa
//   R / L      rotate CW / CCW (manual, until stop)
//   A / S      stop
//   O / F      store current position as full-CCW / full-CW ADC endpoint
//              (immediate, unlike K3NG's interactive two-step flow)
//   Xn         speed preset n=1..4 — accepted and ignored (servo owns speed)

static char buf[12];
static uint8_t len;

void gs232_init() {
  Serial.begin(SERIAL_BAUD);
}

static void reply_azimuth() {
  const int16_t deg = (position_deg10() + 5) / 10;
  char out[8];
  snprintf(out, sizeof(out), "+0%03d", deg);
  Serial.println(out);
}

static void process() {
  if (len == 0) return;
  switch (toupper(buf[0])) {
    case 'C':
      reply_azimuth();
      break;
    case 'M':
      if (len >= 4) {
        const int16_t deg = (buf[1] - '0') * 100 + (buf[2] - '0') * 10 + (buf[3] - '0');
        if (deg >= 0 && deg <= 360) servo_goto(deg * 10);
      }
      break;
    case 'R': servo_manual_cw(); break;
    case 'L': servo_manual_ccw(); break;
    case 'A':
    case 'S': servo_stop(); break;
    case 'O':
      position_cal_save_ccw();
      Serial.println(F("Wrote to memory"));
      break;
    case 'F':
      position_cal_save_cw();
      Serial.println(F("Wrote to memory"));
      break;
    case 'X': break;  // speed presets not needed; servo profiles speed itself
    default:  break;  // ignore unknown commands silently
  }
}

void gs232_poll() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r' || c == '\n') {
      buf[len] = '\0';
      process();
      len = 0;
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
}
