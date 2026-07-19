#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "position.h"

struct cal_t {
  uint16_t magic;
  uint16_t raw_ccw;
  uint16_t raw_cw;
};

static cal_t cal;

void position_init() {
  EEPROM.get(CAL_EEPROM_ADDR, cal);
  if (cal.magic != CAL_EEPROM_MAGIC || cal.raw_cw <= cal.raw_ccw) {
    cal.magic = CAL_EEPROM_MAGIC;
    cal.raw_ccw = CAL_DEFAULT_RAW_CCW;
    cal.raw_cw = CAL_DEFAULT_RAW_CW;
  }
}

uint16_t position_raw() {
  // The two-stage RC filter has already done the real averaging; a short
  // burst average just knocks down ADC quantization noise.
  uint16_t sum = 0;
  for (uint8_t i = 0; i < POS_ADC_SAMPLES; i++) {
    sum += analogRead(PIN_AZ);
  }
  return sum / POS_ADC_SAMPLES;
}

int16_t position_deg10() {
  uint16_t raw = position_raw();
  if (raw <= cal.raw_ccw) return 0;
  if (raw >= cal.raw_cw) return 3600;
  return (int16_t)(((uint32_t)(raw - cal.raw_ccw) * 3600) /
                   (cal.raw_cw - cal.raw_ccw));
}

static void cal_store() {
  EEPROM.put(CAL_EEPROM_ADDR, cal);
}

void position_cal_save_ccw() {
  cal.raw_ccw = position_raw();
  cal_store();
}

void position_cal_save_cw() {
  cal.raw_cw = position_raw();
  cal_store();
}
