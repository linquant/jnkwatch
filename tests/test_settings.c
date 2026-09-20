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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_brightness_clamps);
    RUN_TEST(test_wrist_wake_toggle);
    return UNITY_END();
}
