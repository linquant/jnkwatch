#include "unity.h"
#include "compass.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_normalize_wraps(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 45.0, wf_heading_normalize(45));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 10.0, wf_heading_normalize(370));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 350.0, wf_heading_normalize(-10));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_heading_normalize(720));
}

void test_angle_diff_handles_wrap(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, wf_angle_diff(350, 10));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, -20.0, wf_angle_diff(10, 350));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 90.0, wf_angle_diff(0, 90));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, -179.0, wf_angle_diff(0, 181));
}

void test_cardinal8(void)
{
    TEST_ASSERT_EQUAL_STRING("N", wf_cardinal8(0));
    TEST_ASSERT_EQUAL_STRING("NE", wf_cardinal8(45));
    TEST_ASSERT_EQUAL_STRING("E", wf_cardinal8(90));
    TEST_ASSERT_EQUAL_STRING("S", wf_cardinal8(180));
    TEST_ASSERT_EQUAL_STRING("W", wf_cardinal8(270));
    TEST_ASSERT_EQUAL_STRING("NW", wf_cardinal8(315));
    TEST_ASSERT_EQUAL_STRING("N", wf_cardinal8(350));  /* wraps back to N */
    TEST_ASSERT_EQUAL_STRING("NE", wf_cardinal8(22.5)); /* boundary */
}

void test_normalize_edge_values(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 350.0, wf_heading_normalize(-10.0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 10.0, wf_heading_normalize(370.0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_heading_normalize(360.0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_heading_normalize(720.0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 350.0, wf_heading_normalize(-370.0));
}

void test_angle_diff_sign_and_seam(void)
{
    /* shortest signed turn from `from` to `to`, in (-180, 180] */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, wf_angle_diff(350.0, 10.0));   /* CW  */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, -20.0, wf_angle_diff(10.0, 350.0));  /* CCW */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_angle_diff(90.0, 90.0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 180.0, wf_angle_diff(0.0, 180.0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, -179.0, wf_angle_diff(0.0, 181.0));
}

void test_cardinal8_sector_boundaries(void)
{
    /* each sector spans ±22.5° around its center */
    TEST_ASSERT_EQUAL_STRING("N", wf_cardinal8(22.0));
    TEST_ASSERT_EQUAL_STRING("NE", wf_cardinal8(23.0));
    TEST_ASSERT_EQUAL_STRING("E", wf_cardinal8(112.0));
    TEST_ASSERT_EQUAL_STRING("SE", wf_cardinal8(113.0));
    TEST_ASSERT_EQUAL_STRING("N", wf_cardinal8(337.5)); /* wraps to N sector */
    TEST_ASSERT_EQUAL_STRING("NW", wf_cardinal8(335.0));
    TEST_ASSERT_EQUAL_STRING("N", wf_cardinal8(-1.0));  /* negative normalizes */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_normalize_wraps);
    RUN_TEST(test_normalize_edge_values);
    RUN_TEST(test_angle_diff_handles_wrap);
    RUN_TEST(test_angle_diff_sign_and_seam);
    RUN_TEST(test_cardinal8);
    RUN_TEST(test_cardinal8_sector_boundaries);
    return UNITY_END();
}
