#include "unity.h"
#include "weather.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_init_invalid(void)
{
    wf_weather_t w;
    wf_weather_init(&w);
    TEST_ASSERT_FALSE(w.valid);
    TEST_ASSERT_EQUAL_STRING("", w.location);
}

void test_set_marks_valid(void)
{
    wf_weather_t w;
    wf_weather_init(&w);
    wf_weather_set(&w, 14, WF_WX_RAIN, "Paris");
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL_INT(14, w.temp_c);
    TEST_ASSERT_EQUAL_INT(WF_WX_RAIN, w.cond);
    TEST_ASSERT_EQUAL_STRING("Paris", w.location);
}

void test_negative_temp(void)
{
    wf_weather_t w;
    wf_weather_init(&w);
    wf_weather_set(&w, -7, WF_WX_SNOW, "Oslo");
    TEST_ASSERT_EQUAL_INT(-7, w.temp_c);
    TEST_ASSERT_EQUAL_INT(WF_WX_SNOW, w.cond);
}

void test_cond_clamped(void)
{
    wf_weather_t w;
    wf_weather_init(&w);
    wf_weather_set(&w, 20, 99, "X");         /* out of range */
    TEST_ASSERT_EQUAL_INT(WF_WX_CLOUDY, w.cond);
    wf_weather_set(&w, 20, -1, "X");         /* negative */
    TEST_ASSERT_EQUAL_INT(WF_WX_CLOUDY, w.cond);
    wf_weather_set(&w, 20, WF_WX_STORM, "X");/* valid boundary */
    TEST_ASSERT_EQUAL_INT(WF_WX_STORM, w.cond);
}

void test_null_location(void)
{
    wf_weather_t w;
    wf_weather_init(&w);
    wf_weather_set(&w, 10, WF_WX_CLEAR, NULL);
    TEST_ASSERT_EQUAL_STRING("", w.location);
}

void test_long_location_truncated(void)
{
    wf_weather_t w;
    wf_weather_init(&w);
    wf_weather_set(&w, 10, WF_WX_CLEAR, "AVeryLongCityNameThatOverflows");
    TEST_ASSERT_TRUE(strlen(w.location) < WF_WX_LOC_LEN);
    TEST_ASSERT_EQUAL_CHAR('\0', w.location[WF_WX_LOC_LEN - 1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_invalid);
    RUN_TEST(test_set_marks_valid);
    RUN_TEST(test_negative_temp);
    RUN_TEST(test_cond_clamped);
    RUN_TEST(test_null_location);
    RUN_TEST(test_long_location_truncated);
    return UNITY_END();
}
