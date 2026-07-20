#pragma once
// Host-side Arduino mock for native unit tests. Test code controls the fake
// clock (mock_millis_now), analog input (mock_analog_value), button levels
// (mock_pin_low) and inspects outputs (mock_pin_out, OCR1A/B, Serial.out).

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <deque>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19

#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define _BV(b) (1 << (b))
#define ISR(vec) void vec(void)
#define F(x) x

#define COM1A1 7
#define COM1B1 5
#define WGM10 0
#define WGM12 3
#define CS10 0
#define WGM21 1
#define CS21 1
#define CS20 0
#define OCIE2A 1

// fake AVR registers
inline uint16_t OCR1A, OCR1B;
inline uint8_t TCCR1A, TCCR1B, TCCR2A, TCCR2B, OCR2A, TIMSK2;

// fake clock / pins / ADC
inline uint32_t mock_millis_now = 0;
inline uint32_t millis() { return mock_millis_now; }

inline uint8_t mock_pin_mode[32];
inline uint8_t mock_pin_out[32];
inline uint8_t mock_pin_low[32];  // 1 = button pulls the pin low (pressed)

inline void pinMode(uint8_t pin, uint8_t mode) { mock_pin_mode[pin] = mode; }
inline void digitalWrite(uint8_t pin, uint8_t v) { mock_pin_out[pin] = v; }
inline int digitalRead(uint8_t pin) { return mock_pin_low[pin] ? LOW : HIGH; }

inline uint16_t mock_analog_value = 0;
inline int analogRead(uint8_t) { return mock_analog_value; }

inline void interrupts() {}
inline void noInterrupts() {}

struct MockSerial {
  std::string out;
  std::deque<char> in;
  void begin(unsigned long) {}
  int available() { return (int)in.size(); }
  int read() {
    if (in.empty()) return -1;
    char c = in.front();
    in.pop_front();
    return c;
  }
  void print(const char *s) { out += s; }
  void println(const char *s) { out += s; out += "\r\n"; }
  void inject(const char *s) { while (*s) in.push_back(*s++); }
  void clear() { out.clear(); in.clear(); }
};
inline MockSerial Serial;
