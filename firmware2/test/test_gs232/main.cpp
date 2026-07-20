#include <unity.h>
#include "../helpers.h"

void setUp() {
  Serial.clear();
  EEPROM.clear();
  mock_millis_now = 1000;
  dds_init();
  position_init();
  servo_stop();
  gs232_init();
  mock_analog_value = ANALOG_AT_MID;  // ~180 deg
}
void tearDown() {}

void test_c_reports_azimuth() {
  Serial.inject("C\r");
  gs232_poll();
  TEST_ASSERT_EQUAL_STRING("+0180\r\n", Serial.out.c_str());
}

void test_m_starts_goto() {
  Serial.inject("M240\r");
  gs232_poll();
  TEST_ASSERT_EQUAL(SERVO_GOTO, servo_get_state());
  TEST_ASSERT_EQUAL_INT16(2400, servo_get_target_deg10());
}

void test_m_out_of_range_ignored() {
  Serial.inject("M999\r");
  gs232_poll();
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
}

void test_r_l_s_lowercase_too() {
  Serial.inject("L\r");
  gs232_poll();
  TEST_ASSERT_EQUAL(SERVO_MAN_CCW, servo_get_state());
  Serial.inject("s\r");
  gs232_poll();
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
  Serial.inject("r\r");
  gs232_poll();
  TEST_ASSERT_EQUAL(SERVO_MAN_CW, servo_get_state());
  Serial.inject("A\r");
  gs232_poll();
  TEST_ASSERT_EQUAL(SERVO_IDLE, servo_get_state());
}

void test_o_f_calibrate() {
  mock_analog_value = ANALOG_AT_CCW;
  Serial.inject("O\r");
  gs232_poll();
  TEST_ASSERT_NOT_NULL(strstr(Serial.out.c_str(), "Wrote to memory"));
  mock_analog_value = ANALOG_AT_CW;
  Serial.inject("F\r");
  gs232_poll();
  mock_analog_value = ANALOG_AT_CCW;
  TEST_ASSERT_INT16_WITHIN(10, 0, position_deg10());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_c_reports_azimuth);
  RUN_TEST(test_m_starts_goto);
  RUN_TEST(test_m_out_of_range_ignored);
  RUN_TEST(test_r_l_s_lowercase_too);
  RUN_TEST(test_o_f_calibrate);
  return UNITY_END();
}
