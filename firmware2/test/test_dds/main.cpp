#include <unity.h>
#include "../helpers.h"

void TIMER2_COMPA_vect();  // the DDS sample ISR, callable natively

void setUp() {
  mock_millis_now = 1000;
  dds_init();
}
void tearDown() {}

void test_idle_outputs_midscale_and_muted() {
  TIMER2_COMPA_vect();
  TEST_ASSERT_EQUAL_UINT16(128, OCR1A);
  TEST_ASSERT_EQUAL_UINT16(128, OCR1B);
  TEST_ASSERT_TRUE(dds_idle());
  run_dds_ms(10);
  TEST_ASSERT_EQUAL(LOW, mock_pin_out[PIN_AMP_MUTE]);
  TEST_ASSERT_EQUAL(LOW, mock_pin_out[PIN_AMP_STBY]);
}

void test_soft_start_reaches_full_amplitude() {
  dds_run(600, DDS_CW);
  run_dds_ms(400);
  TEST_ASSERT_LESS_THAN(255, dds_get_amp());  // still ramping at 400 ms
  TEST_ASSERT_EQUAL(HIGH, mock_pin_out[PIN_AMP_MUTE]);
  run_dds_ms(200);
  TEST_ASSERT_EQUAL(255, dds_get_amp());      // full by ~510 ms
}

void test_cw_quadrature_cos_leads() {
  dds_run(600, DDS_CW);
  run_dds_ms(600);
  TIMER2_COMPA_vect();  // first sample after phase reset: sin ~ 0 rising
  TEST_ASSERT_UINT16_WITHIN(8, 128, OCR1A);
  TEST_ASSERT_GREATER_THAN(240, OCR1B);       // cos at +max: leads by 90
}

void test_ccw_quadrature_cos_lags() {
  dds_run(600, DDS_CCW);
  run_dds_ms(600);
  TIMER2_COMPA_vect();
  TEST_ASSERT_UINT16_WITHIN(8, 128, OCR1A);
  TEST_ASSERT_LESS_THAN(16, OCR1B);           // cos at -max: lags by 90
}

void test_vf_law_reduces_amplitude_at_low_freq() {
  dds_run(160, DDS_CW);  // 16 Hz taper case
  run_dds_ms(600);
  TEST_ASSERT_UINT8_WITHIN(6, 106, dds_get_amp());
}

void test_stop_ramp_is_fast() {
  dds_run(600, DDS_CW);
  run_dds_ms(600);
  dds_stop();
  const uint32_t t0 = mock_millis_now;
  while (dds_get_amp() > 0 && mock_millis_now - t0 < 1000) {
    run_dds_ms(1);
  }
  TEST_ASSERT_LESS_THAN(250, mock_millis_now - t0);  // stop means stop
  TEST_ASSERT_TRUE(dds_idle());
}

void test_reversal_ramps_through_zero() {
  dds_run(600, DDS_CW);
  run_dds_ms(600);
  TEST_ASSERT_EQUAL(DDS_CW, dds_get_dir());

  dds_run(600, DDS_CCW);  // reversal request at full amplitude
  bool reached_zero = false;
  for (int i = 0; i < 1500; i++) {
    run_dds_ms(1);
    if (!reached_zero && dds_get_amp() > 0) {
      TEST_ASSERT_EQUAL_MESSAGE(DDS_CW, dds_get_dir(),
                                "direction flipped while energized");
    }
    if (dds_get_amp() == 0) reached_zero = true;
  }
  TEST_ASSERT_TRUE(reached_zero);
  TEST_ASSERT_EQUAL(DDS_CCW, dds_get_dir());
  TEST_ASSERT_EQUAL(255, dds_get_amp());
}

void test_stop_cancels_pending_reversal() {
  dds_run(600, DDS_CW);
  run_dds_ms(600);
  dds_run(600, DDS_CCW);
  run_dds_ms(10);
  dds_stop();  // user changed their mind mid-reversal
  run_dds_ms(600);
  TEST_ASSERT_TRUE(dds_idle());
  TEST_ASSERT_EQUAL(0, dds_get_amp());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_idle_outputs_midscale_and_muted);
  RUN_TEST(test_soft_start_reaches_full_amplitude);
  RUN_TEST(test_cw_quadrature_cos_leads);
  RUN_TEST(test_ccw_quadrature_cos_lags);
  RUN_TEST(test_vf_law_reduces_amplitude_at_low_freq);
  RUN_TEST(test_stop_ramp_is_fast);
  RUN_TEST(test_reversal_ramps_through_zero);
  RUN_TEST(test_stop_cancels_pending_reversal);
  return UNITY_END();
}
