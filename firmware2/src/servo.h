#pragma once
#include <stdint.h>

enum servo_state : uint8_t { SERVO_IDLE, SERVO_GOTO, SERVO_MAN_CW, SERVO_MAN_CCW };

void servo_goto(int16_t target_deg10);
void servo_manual_cw();
void servo_manual_ccw();
void servo_stop();
void servo_tick(uint32_t now_ms);
servo_state servo_get_state();
int16_t servo_get_target_deg10();
