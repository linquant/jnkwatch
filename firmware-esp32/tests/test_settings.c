#include "unity.h"
#include "settings.h"

void setUp(void) {}
void tearDown(void) {}

void test_defaults(void)
{
    wf_settings_t s;
    wf_settings_init(&s);
    TEST_ASSERT_EQUAL_INT(80, s.brightness);
    TEST_ASSERT_TRUE(s.wrist_wake);
    TEST_ASSERT_EQUAL_INT(1, s.wrist_sens);   /* Moyenne */
    TEST_ASSERT_EQUAL_INT(WF_SLEEP_10S, s.sleep_ms);
}

void test_sleep_snaps_to_allowed(void)
{
    wf_settings_t s;
    wf_settings_init(&s);
    TEST_ASSERT_EQUAL_INT(WF_SLEEP_5S,  wf_settings_set_sleep_ms(&s, 5000));
    TEST_ASSERT_EQUAL_INT(WF_SLEEP_5S,  s.sleep_ms);
    TEST_ASSERT_EQUAL_INT(WF_SLEEP_10S, wf_settings_set_sleep_ms(&s, 10000));
    TEST_ASSERT_EQUAL_INT(WF_SLEEP_5S,  wf_settings_set_sleep_ms(&s, 6000));  /* < midpoint */
    TEST_ASSERT_EQUAL_INT(WF_SLEEP_10S, wf_settings_set_sleep_ms(&s, 99999)); /* clamps up */
}

void test_brightness_clamps(void)
{
    wf_settings_t s;
    wf_settings_init(&s);
    TEST_ASSERT_EQUAL_INT(55, wf_settings_set_brightness(&s, 55));
    TEST_ASSERT_EQUAL_INT(55, s.brightness);
    TEST_ASSERT_EQUAL_INT(WF_BRIGHT_MIN, wf_settings_set_brightness(&s, 0));
    TEST_ASSERT_EQUAL_INT(WF_BRIGHT_MIN, wf_settings_set_brightness(&s, -20));
    TEST_ASSERT_EQUAL_INT(WF_BRIGHT_MAX, wf_settings_set_brightness(&s, 250));
}

void test_wrist_wake_toggle(void)
{
    wf_settings_t s;
    wf_settings_init(&s);
    wf_settings_set_wrist_wake(&s, false);
    TEST_ASSERT_FALSE(s.wrist_wake);
    wf_settings_set_wrist_wake(&s, true);
    TEST_ASSERT_TRUE(s.wrist_wake);
}

void test_wrist_sens_clamps(void)
{
    wf_settings_t s;
    wf_settings_init(&s);
    TEST_ASSERT_EQUAL_INT(0, wf_settings_set_wrist_sens(&s, 0));
    TEST_ASSERT_EQUAL_INT(0, s.wrist_sens);
    TEST_ASSERT_EQUAL_INT(2, wf_settings_set_wrist_sens(&s, 2));
    TEST_ASSERT_EQUAL_INT(2, s.wrist_sens);
    TEST_ASSERT_EQUAL_INT(0, wf_settings_set_wrist_sens(&s, -5));  /* clamps up */
    TEST_ASSERT_EQUAL_INT(2, wf_settings_set_wrist_sens(&s, 99));  /* clamps down */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_sleep_snaps_to_allowed);
    RUN_TEST(test_brightness_clamps);
    RUN_TEST(test_wrist_wake_toggle);
    RUN_TEST(test_wrist_sens_clamps);
    return UNITY_END();
}
