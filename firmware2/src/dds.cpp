#include <Arduino.h>
#include "config.h"
#include "dds.h"

static const int8_t SINE[256] PROGMEM = {
     0,    3,    6,    9,   12,   16,   19,   22,
    25,   28,   31,   34,   37,   40,   43,   46,
    49,   51,   54,   57,   60,   63,   65,   68,
    71,   73,   76,   78,   81,   83,   85,   88,
    90,   92,   94,   96,   98,  100,  102,  104,
   106,  107,  109,  111,  112,  113,  115,  116,
   117,  118,  120,  121,  122,  122,  123,  124,
   125,  125,  126,  126,  126,  127,  127,  127,
   127,  127,  127,  127,  126,  126,  126,  125,
   125,  124,  123,  122,  122,  121,  120,  118,
   117,  116,  115,  113,  112,  111,  109,  107,
   106,  104,  102,  100,   98,   96,   94,   92,
    90,   88,   85,   83,   81,   78,   76,   73,
    71,   68,   65,   63,   60,   57,   54,   51,
    49,   46,   43,   40,   37,   34,   31,   28,
    25,   22,   19,   16,   12,    9,    6,    3,
     0,   -3,   -6,   -9,  -12,  -16,  -19,  -22,
   -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
   -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,
   -71,  -73,  -76,  -78,  -81,  -83,  -85,  -88,
   -90,  -92,  -94,  -96,  -98, -100, -102, -104,
  -106, -107, -109, -111, -112, -113, -115, -116,
  -117, -118, -120, -121, -122, -122, -123, -124,
  -125, -125, -126, -126, -126, -127, -127, -127,
  -127, -127, -127, -127, -126, -126, -126, -125,
  -125, -124, -123, -122, -122, -121, -120, -118,
  -117, -116, -115, -113, -112, -111, -109, -107,
  -106, -104, -102, -100,  -98,  -96,  -94,  -92,
   -90,  -88,  -85,  -83,  -81,  -78,  -76,  -73,
   -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
   -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,
   -25,  -22,  -19,  -16,  -12,   -9,   -6,   -3
};

// Timer2 CTC: 16 MHz / 32 / (61+1) = 8064.5 Hz sample clock.
#define DDS_SAMPLE_TOP 61
// phase_inc = dHz * 65536 / (10 * 8064.5) = dHz * 0.8127 = (dHz * 53266) >> 16
#define DDS_INC_MUL 53266UL

static volatile uint16_t phase;
static volatile uint16_t phase_inc;
static volatile uint8_t  amp;         // current amplitude, slewed in dds_tick
static volatile uint8_t  quad_offset; // +64 (CW) or -64 (CCW) table entries

static uint8_t  amp_target;
static uint32_t last_slew_ms;

ISR(TIMER2_COMPA_vect) {
  phase += phase_inc;
  const uint8_t i = phase >> 8;
  const int8_t s = (int8_t)pgm_read_byte(&SINE[i]);
  const int8_t c = (int8_t)pgm_read_byte(&SINE[(uint8_t)(i + quad_offset)]);
  OCR1A = 128 + (((int16_t)s * amp) >> 8);
  OCR1B = 128 + (((int16_t)c * amp) >> 8);
}

void dds_init() {
  pinMode(PIN_DRIVE_SIN, OUTPUT);
  pinMode(PIN_DRIVE_COS, OUTPUT);
  pinMode(PIN_AMP_MUTE, OUTPUT);
  pinMode(PIN_AMP_STBY, OUTPUT);
  digitalWrite(PIN_AMP_MUTE, LOW);   // LOW = muted (pull-downs hold this in reset)
  digitalWrite(PIN_AMP_STBY, LOW);   // LOW = standby

  // Timer1: fast PWM 8-bit, non-inverting on OC1A/OC1B, no prescale (62.5 kHz)
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 128;
  OCR1B = 128;

  // Timer2: CTC, /32 prescaler, interrupt on compare A
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21) | _BV(CS20);
  OCR2A  = DDS_SAMPLE_TOP;
  TIMSK2 = _BV(OCIE2A);
}

static uint8_t vf_amplitude(uint16_t freq_dhz) {
  uint16_t a = DRIVE_AMP_FLOOR +
               (((uint32_t)(freq_dhz - DRIVE_FREQ_MIN_DHZ) * DRIVE_AMP_SLOPE) >> 8);
  return a > 255 ? 255 : (uint8_t)a;
}

void dds_run(uint16_t freq_dhz, dds_dir dir) {
  if (freq_dhz < DRIVE_FREQ_MIN_DHZ) freq_dhz = DRIVE_FREQ_MIN_DHZ;
  if (freq_dhz > DRIVE_FREQ_MAX_DHZ) freq_dhz = DRIVE_FREQ_MAX_DHZ;
  const uint16_t inc = ((uint32_t)freq_dhz * DDS_INC_MUL) >> 16;
  const uint8_t q = (dir == DDS_CW) ? 64 : (uint8_t)-64;
  noInterrupts();
  phase_inc = inc;
  quad_offset = q;
  interrupts();
  amp_target = vf_amplitude(freq_dhz);
}

void dds_stop() {
  amp_target = 0;
}

bool dds_idle() {
  return amp_target == 0 && amp == 0;
}

void dds_tick(uint32_t now_ms) {
  if (now_ms - last_slew_ms < DRIVE_AMP_SLEW_MS) return;
  last_slew_ms = now_ms;

  uint8_t a = amp;
  if (a < amp_target) a++;
  else if (a > amp_target) a--;
  else return;
  amp = a;

  // Amp powered whenever we drive; muted and in standby when silent.
  digitalWrite(PIN_AMP_STBY, a == 0 ? LOW : HIGH);
  digitalWrite(PIN_AMP_MUTE, a == 0 ? LOW : HIGH);
}
