#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "position.h"

struct cal_t {
  uint16_t magic;
  uint16_t r_ccw;  // ohms at the full-CCW stop
  uint16_t r_cw;   // ohms at the full-CW stop
};

static cal_t cal;

static bool cal_valid(const cal_t &c) {
  const int16_t span = (int16_t)c.r_cw - (int16_t)c.r_ccw;
  return c.magic == CAL_EEPROM_MAGIC && (span > 100 || span < -100);
}

void position_init() {
  EEPROM.get(CAL_EEPROM_ADDR, cal);
  if (!cal_valid(cal)) {
    cal.magic = CAL_EEPROM_MAGIC;
    cal.r_ccw = CAL_DEFAULT_R_CCW;
    cal.r_cw = CAL_DEFAULT_R_CW;
  }
}

uint16_t position_raw() {
  // 16x oversample: the two-stage RC removed the fast noise; the residual
  // ripple acts as dither, so decimating the sum yields a 12-bit reading.
  uint16_t sum = 0;
  for (uint8_t i = 0; i < POS_ADC_SAMPLES; i++) {
    sum += analogRead(PIN_AZ);
  }
  return sum >> 2;  // 0..4092
}

// Invert the divider: raw = 4092*R/(R+Rf)  ->  R = Rf*raw/(4092-raw)
uint16_t position_ohms() {
  uint16_t raw = position_raw();
  if (raw > 4000) raw = 4000;  // open track / broken wire: clamp the division
  return (uint16_t)(((uint32_t)POS_R_FIXED * raw) / (4092u - raw));
}

int16_t position_deg10() {
  const int32_t r = position_ohms();
  const int32_t span = (int32_t)cal.r_cw - (int32_t)cal.r_ccw;
  int32_t deg10 = ((r - (int32_t)cal.r_ccw) * 3600) / span;
  if (deg10 < 0) deg10 = 0;
  if (deg10 > 3600) deg10 = 3600;
  return (int16_t)deg10;
}

static void cal_store() {
  EEPROM.put(CAL_EEPROM_ADDR, cal);
}

void position_cal_save_ccw() {
  cal.r_ccw = position_ohms();
  cal_store();
}

void position_cal_save_cw() {
  cal.r_cw = position_ohms();
  cal_store();
}
