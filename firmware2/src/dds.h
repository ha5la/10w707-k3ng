#pragma once
#include <stdint.h>

// Two-channel quadrature sine DDS on Timer1 PWM (D9/D10), sample clock on
// Timer2. Channel B leads or lags channel A by 90 degrees depending on
// direction, which is what reverses the motor — no relays involved.

enum dds_dir : uint8_t { DDS_CW, DDS_CCW };

void dds_init();
// Run at freq_dhz (deci-Hz, clamped to DRIVE_FREQ_MIN/MAX) in the given
// direction. Amplitude follows the V/f law and slews softly.
void dds_run(uint16_t freq_dhz, dds_dir dir);
// Ramp amplitude to zero; outputs idle at midscale once the ramp finishes.
void dds_stop();
// Call from loop(); paces the amplitude slew and the amp MUTE/STBY pins.
void dds_tick(uint32_t now_ms);
// True once dds_stop() has fully ramped down (safe to change direction).
bool dds_idle();
