#include <Arduino.h>
#include "config.h"
#include "dds.h"
#include "position.h"
#include "servo.h"
#include "gs232.h"
#include "display.h"
#include "buttons.h"

void setup() {
  dds_init();       // first: forces amp into standby/mute before anything else
  position_init();
  gs232_init();
  buttons_init();
  display_init();
}

void loop() {
  const uint32_t now = millis();
  gs232_poll();
  buttons_tick(now);
  servo_tick(now);
  dds_tick(now);
  display_tick(now);
}
