#include <Arduino.h>
#include <LiquidCrystal.h>
#include "config.h"
#include "display.h"
#include "position.h"
#include "servo.h"

static LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E,
                         PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
static uint32_t last_ms;

void display_init() {
  lcd.begin(16, 2);
  lcd.print(F("10W707 rotator"));
}

void display_tick(uint32_t now_ms) {
  if (now_ms - last_ms < DISPLAY_UPDATE_MS) return;
  last_ms = now_ms;

  char line[17];
  const int16_t deg = (position_deg10() + 5) / 10;

  snprintf(line, sizeof(line), "Azimuth %3d\xDF    ", deg);
  lcd.setCursor(0, 0);
  lcd.print(line);

  lcd.setCursor(0, 1);
  switch (servo_get_state()) {
    case SERVO_IDLE:
      lcd.print(F("Idle            "));
      break;
    case SERVO_GOTO:
      snprintf(line, sizeof(line), "Goto %3d\xDF       ",
               (servo_get_target_deg10() + 5) / 10);
      lcd.print(line);
      break;
    case SERVO_MAN_CW:
      lcd.print(F("Manual CW  >>   "));
      break;
    case SERVO_MAN_CCW:
      lcd.print(F("Manual CCW <<   "));
      break;
  }
}
