#pragma once
#include <stdint.h>

void position_init();
// Azimuth in tenths of a degree, 0..3600, linear in the resistance domain.
int16_t position_deg10();
// Pot track segment resistance in ohms, exact inverse of the divider.
uint16_t position_ohms();
// Store current raw reading as the full-CCW / full-CW endpoint (EEPROM).
void position_cal_save_ccw();
void position_cal_save_cw();
