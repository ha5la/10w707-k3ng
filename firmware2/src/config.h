#pragma once

// ---- Pin map (Arduino Nano, ATmega328P) -------------------------------------
// Timer1 PWM carriers -> RC low-pass -> attenuator -> TDA7294 inputs.
// D9/D10 are fixed by hardware (OC1A/OC1B); everything else matches the
// existing K3NG-era wiring so the perfboard changes stay minimal.

#define PIN_DRIVE_SIN     9   // OC1A — winding W1 (tower wire 1)
#define PIN_DRIVE_COS     10  // OC1B — winding W2 (tower wire 4)

// TDA7294 MUTE + STBY driven together from one pin (datasheet Fig. 17
// single-signal control): the per-pin RCs (10k+10u mute, 22k+10u stby)
// sequence standby-on before mute-release, and the reverse on shutdown.
// HIGH = play; a 10k pull-down on the line keeps the amps silent through
// MCU reset. D7 is free.
#define PIN_AMP_ENABLE    6

#define PIN_AZ            A0  // position pot via two-stage RC filter

#define PIN_BUTTON_CW     A2
#define PIN_BUTTON_CCW    A3

// 16x2 LCD, 4-bit, K3NG standard wiring
#define PIN_LCD_RS        12
#define PIN_LCD_E         11
#define PIN_LCD_D4        5
#define PIN_LCD_D5        4
#define PIN_LCD_D6        3
#define PIN_LCD_D7        2

// ---- Serial -----------------------------------------------------------------
#define SERIAL_BAUD       9600  // GS-232A subset, rotctld model 603

// ---- Drive ------------------------------------------------------------------
// Frequencies in deci-Hz. Motor design point: 20 V / 60 Hz (0.3 V/Hz).
// Quadrature drive holds 90 degrees at every frequency, so the usable range is
// limited only by slip at the bottom and design flux at the top.
#define DRIVE_FREQ_MIN_DHZ   50   //  5 Hz crawl (approach speed)
#define DRIVE_FREQ_MAX_DHZ  600   // 60 Hz full speed

// V/f amplitude law: linear from AMP_FLOOR at FREQ_MIN to 255 at FREQ_MAX
// (exact at both endpoints). AMP 255 = full output = 18 V RMS per winding
// (52 V rail). AMP_FLOOR compensates the ~6 ohm winding IR at crawl speed.
#define DRIVE_AMP_FLOOR      70

// Soft start: +1 amplitude step per 2 ms tick -> ~0.5 s 0..255.
// Stop/limit ramp-down is faster (stop means stop): -3 per tick -> ~0.17 s.
// Direction reversals always ramp down through zero before the phase flips.
#define DRIVE_AMP_SLEW_MS    2
#define DRIVE_AMP_DOWN_STEP  3

// ---- Servo ------------------------------------------------------------------
#define SERVO_TICK_MS        20
#define SERVO_DEADBAND_DEG10 20   // +/- 2.0 degrees
#define SERVO_TAPER_DEG10    150  // start slowing below 15 degrees of error
// Block manual drive within 6 deg of an end stop: covers the ramp-down coast
// (~7 deg/s during the 0.17 s stop ramp) plus the ~0.3 s position-filter lag.
#define SERVO_SOFT_LIMIT_DEG10 60

// ---- Position ---------------------------------------------------------------
// Quadrature-drive sensing: motor common (tower 5 = pot wiper) is GND; A0
// reads tower wire 2 through the divider 5V -> 1k -> A0 -> R(segment) -> GND.
// V is a hyperbola in R; position_ohms() inverts it exactly, and R is linear
// in angle, so two-point calibration stays exact. Wire 3 is spare.
#define POS_ADC_SAMPLES      16    // oversample burst, sum decimated to 12 bit
// Divider top = first filter resistor. 820R chosen over 1.2k: ~18% more span
// (0.57-2.61 V for the 106-896 ohm swing) at equal worst-case resolution.
// An inexact value only scales R linearly; two-point calibration absorbs it.
#define POS_R_FIXED          820
// Defaults from shop measurements (2026-07-19): R(2-5) swings 106..896 ohm
// between the end stops. O / F store the real endpoints (in ohms) in EEPROM.
#define CAL_DEFAULT_R_CCW    106
#define CAL_DEFAULT_R_CW     896
#define CAL_EEPROM_ADDR      0
#define CAL_EEPROM_MAGIC     0x4B53  // "KS" — bumped: calibration now in ohms

// ---- Display ----------------------------------------------------------------
#define DISPLAY_UPDATE_MS    250

// ---- Buttons ----------------------------------------------------------------
#define BUTTON_TICK_MS       10
#define BUTTON_DEBOUNCE_TICKS 3
