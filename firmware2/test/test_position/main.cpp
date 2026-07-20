#include <unity.h>
#include "../helpers.h"

void setUp() {
  EEPROM.clear();
  position_init();
}
void tearDown() {}

void test_ohms_inversion() {
  mock_analog_value = ANALOG_AT_MID;  // raw 1552 -> 820*1552/2540 ~ 501 ohm
  TEST_ASSERT_UINT16_WITHIN(3, 501, position_ohms());
}

void test_endpoints_and_linear_midpoint() {
  mock_analog_value = ANALOG_AT_CCW;
  TEST_ASSERT_INT16_WITHIN(10, 0, position_deg10());
  mock_analog_value = ANALOG_AT_MID;
  TEST_ASSERT_INT16_WITHIN(15, 1800, position_deg10());  // linear in R, not in V
  mock_analog_value = ANALOG_AT_CW;
  TEST_ASSERT_INT16_WITHIN(10, 3600, position_deg10());
}

void test_open_wire_clamps() {
  mock_analog_value = 1023;  // broken track -> raw at full scale
  TEST_ASSERT_EQUAL_INT16(3600, position_deg10());
}

void test_calibration_normal_and_reversed() {
  mock_analog_value = ANALOG_AT_CCW;
  position_cal_save_ccw();
  mock_analog_value = ANALOG_AT_CW;
  position_cal_save_cw();
  mock_analog_value = ANALOG_AT_CCW;
  TEST_ASSERT_INT16_WITHIN(10, 0, position_deg10());

  // reversed rotor sense: CCW stop at high resistance
  mock_analog_value = ANALOG_AT_CW;
  position_cal_save_ccw();
  mock_analog_value = ANALOG_AT_CCW;
  position_cal_save_cw();
  mock_analog_value = ANALOG_AT_CW;
  TEST_ASSERT_INT16_WITHIN(10, 0, position_deg10());
  mock_analog_value = ANALOG_AT_CCW;
  TEST_ASSERT_INT16_WITHIN(10, 3600, position_deg10());
}

void test_calibration_survives_reinit() {
  mock_analog_value = ANALOG_AT_MID;
  position_cal_save_ccw();
  mock_analog_value = ANALOG_AT_CW;
  position_cal_save_cw();
  position_init();  // reload from (mock) EEPROM
  mock_analog_value = ANALOG_AT_MID;
  TEST_ASSERT_INT16_WITHIN(10, 0, position_deg10());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_ohms_inversion);
  RUN_TEST(test_endpoints_and_linear_midpoint);
  RUN_TEST(test_open_wire_clamps);
  RUN_TEST(test_calibration_normal_and_reversed);
  RUN_TEST(test_calibration_survives_reinit);
  return UNITY_END();
}
