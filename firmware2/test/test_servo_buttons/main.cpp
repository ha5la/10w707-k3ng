#include <unity.h>
#include "../helpers.h"

void setUp() {
  mock_millis_now = 1000;
  memset(mock_pin_low, 0, sizeof(mock_pin_low));
  EEPROM.clear();
  dds_init();
  position_init();
  buttons_init();
  servo_stop();
  mock_analog_value = ANALOG_AT_MID;  // ~180 deg
  run_ms(50);
}
void tearDown() {}

void test_goto_tapers_near_target_full_speed_far() {
  servo_goto(1900);  // 10 deg away -> inside taper zone
  run_ms(50);
  TEST_ASSERT_UINT16_WITHIN(40, 416, dds_get_freq_dhz());
  servo_goto(3300);  // 150 deg away -> full speed
  run_ms(50);
  TEST_ASSERT_EQUAL_UINT16(600, dds_get_freq_dhz());
}

void test_goto_deadband_stops() {
  servo_goto(1810);  // 1 deg away: inside the +/-2 deg deadband
  run_ms(50);
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
}

void test_soft_limits_block_manual_near_stops() {
  mock_analog_value = ANALOG_AT_3550;  // ~355 deg: within 6 deg of the CW stop
  servo_manual_cw();
  run_ms(50);
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
  servo_manual_ccw();  // away from the stop: allowed
  run_ms(50);
  TEST_ASSERT_EQUAL(SERVO_MAN_CCW, servo_get_state());
  servo_stop();
}

void test_button_press_overrides_goto() {
  servo_goto(3300);
  run_ms(50);
  TEST_ASSERT_EQUAL(SERVO_GOTO, servo_get_state());
  press_cw();
  run_ms(60);
  TEST_ASSERT_EQUAL(SERVO_MAN_CW, servo_get_state());
  release_cw();
  run_ms(60);
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
}

void test_both_buttons_stop_and_latch_until_released() {
  press_cw();
  run_ms(60);
  TEST_ASSERT_EQUAL(SERVO_MAN_CW, servo_get_state());
  press_ccw();  // second button while first held: stop, don't reverse
  run_ms(60);
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
  release_ccw();  // one still held: stay stopped
  run_ms(60);
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
  release_cw();  // full release, then a fresh press works again
  run_ms(60);
  press_cw();
  run_ms(60);
  TEST_ASSERT_EQUAL(SERVO_MAN_CW, servo_get_state());
}

void test_quick_manual_reversal_is_sequenced() {
  press_cw();
  run_ms(700);  // full speed CW
  TEST_ASSERT_EQUAL(255, dds_get_amp());
  release_cw();
  press_ccw();  // near-simultaneous flip
  bool reached_zero = false;
  for (int i = 0; i < 1500; i++) {
    run_ms(1);
    if (!reached_zero && dds_get_amp() > 0) {
      TEST_ASSERT_EQUAL_MESSAGE(DDS_CW, dds_get_dir(),
                                "field reversed while energized");
    }
    if (dds_get_amp() == 0) reached_zero = true;
  }
  TEST_ASSERT_TRUE(reached_zero);
  TEST_ASSERT_EQUAL(SERVO_MAN_CCW, servo_get_state());
  TEST_ASSERT_EQUAL(DDS_CCW, dds_get_dir());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_goto_tapers_near_target_full_speed_far);
  RUN_TEST(test_goto_deadband_stops);
  RUN_TEST(test_soft_limits_block_manual_near_stops);
  RUN_TEST(test_button_press_overrides_goto);
  RUN_TEST(test_both_buttons_stop_and_latch_until_released);
  RUN_TEST(test_quick_manual_reversal_is_sequenced);
  return UNITY_END();
}
