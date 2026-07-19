#pragma once
#include <stdint.h>

void position_init();
// Azimuth in tenths of a degree, 0..3600, from the calibrated pot reading.
int16_t position_deg10();
// Raw averaged ADC value (for calibration).
uint16_t position_raw();
// Store current raw reading as the full-CCW / full-CW endpoint (EEPROM).
void position_cal_save_ccw();
void position_cal_save_cw();
