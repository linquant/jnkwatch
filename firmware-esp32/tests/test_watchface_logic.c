#include "unity.h"
#include "watchface_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* ---- fractions of a turn (clockwise from 12 o'clock) ---- */

void test_second_fraction(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.00, wf_second_fraction(0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, wf_second_fraction(15));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.50, wf_second_fraction(30));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.75, wf_second_fraction(45));
}

void test_minute_fraction_is_smooth_with_seconds(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_minute_fraction(0, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, wf_minute_fraction(30, 0));
    /* 15 min 30 s -> (15 + 0.5)/60 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.5 / 60.0, wf_minute_fraction(15, 30));
}

void test_hour_fraction_wraps_at_12_and_is_smooth(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_hour_fraction(0, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, wf_hour_fraction(3, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_hour_fraction(12, 0));  /* noon */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, wf_hour_fraction(15, 0)); /* 3pm  */
    /* 6:30 -> half past six is 6.5/12 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.5 / 12.0, wf_hour_fraction(6, 30));
}

/* ---- hand geometry ---- */

void test_hand_points_up_at_twelve(void)
{
    wf_hand_t h = wf_hand_geometry(120, 120, 0.0, 100, 20);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 120.0, h.x1); /* tip straight up */
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 20.0, h.y1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 120.0, h.x0); /* tail below center */
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 140.0, h.y0);
}

void test_hand_points_right_at_quarter(void)
{
    wf_hand_t h = wf_hand_geometry(120, 120, 0.25, 100, 20);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 220.0, h.x1); /* tip to the right */
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 120.0, h.y1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 100.0, h.x0); /* tail to the left */
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 120.0, h.y0);
}

/* ---- formatting ---- */

void test_format_time_zero_pads(void)
{
    char buf[8];
    wf_format_time(buf, sizeof(buf), 9, 0);
    TEST_ASSERT_EQUAL_STRING("09:00", buf);
    wf_format_time(buf, sizeof(buf), 13, 5);
    TEST_ASSERT_EQUAL_STRING("13:05", buf);
}

void test_format_date_uses_month_abbrev(void)
{
    char buf[8];
    wf_format_date(buf, sizeof(buf), 14, 8); /* month index 8 = September */
    TEST_ASSERT_EQUAL_STRING("14 SEP", buf);
    wf_format_date(buf, sizeof(buf), 1, 0);
    TEST_ASSERT_EQUAL_STRING("01 JAN", buf);
}

void test_format_steps_groups_thousands(void)
{
    char buf[16];
    wf_format_steps(buf, sizeof(buf), 0);
    TEST_ASSERT_EQUAL_STRING("0", buf);
    wf_format_steps(buf, sizeof(buf), 240);
    TEST_ASSERT_EQUAL_STRING("240", buf);
    wf_format_steps(buf, sizeof(buf), 6240);
    TEST_ASSERT_EQUAL_STRING("6 240", buf);
    wf_format_steps(buf, sizeof(buf), 12345);
    TEST_ASSERT_EQUAL_STRING("12 345", buf);
    wf_format_steps(buf, sizeof(buf), 1000000);
    TEST_ASSERT_EQUAL_STRING("1 000 000", buf);
}

/* ---- Casio digital helpers ---- */

void test_seven_seg_mask_known_digits(void)
{
    TEST_ASSERT_EQUAL_HEX(WF_SEG_A | WF_SEG_B | WF_SEG_C | WF_SEG_D | WF_SEG_E | WF_SEG_F,
                          wf_seven_seg_mask(0));
    TEST_ASSERT_EQUAL_HEX(WF_SEG_B | WF_SEG_C, wf_seven_seg_mask(1));
    TEST_ASSERT_EQUAL_HEX(WF_SEG_A | WF_SEG_B | WF_SEG_G | WF_SEG_E | WF_SEG_D,
                          wf_seven_seg_mask(2));
    TEST_ASSERT_EQUAL_HEX(WF_SEG_F | WF_SEG_G | WF_SEG_B | WF_SEG_C,
                          wf_seven_seg_mask(4));
    TEST_ASSERT_EQUAL_HEX(0x7F, wf_seven_seg_mask(8)); /* all seven segments */
    TEST_ASSERT_EQUAL_HEX(WF_SEG_A | WF_SEG_B | WF_SEG_C | WF_SEG_D | WF_SEG_F | WF_SEG_G,
                          wf_seven_seg_mask(9));
}

void test_seven_seg_mask_rejects_out_of_range(void)
{
    TEST_ASSERT_EQUAL_HEX(0, wf_seven_seg_mask(-1));
    TEST_ASSERT_EQUAL_HEX(0, wf_seven_seg_mask(10));
}

void test_weekday_abbrev(void)
{
    TEST_ASSERT_EQUAL_STRING("SUN", wf_weekday_abbrev(0));
    TEST_ASSERT_EQUAL_STRING("MON", wf_weekday_abbrev(1));
    TEST_ASSERT_EQUAL_STRING("SAT", wf_weekday_abbrev(6));
    TEST_ASSERT_EQUAL_STRING("???", wf_weekday_abbrev(7));
}

void test_format_month_day_no_leading_zeros(void)
{
    char buf[8];
    wf_format_month_day(buf, sizeof(buf), 8, 14); /* Sep 14 */
    TEST_ASSERT_EQUAL_STRING("9-14", buf);
    wf_format_month_day(buf, sizeof(buf), 0, 1);
    TEST_ASSERT_EQUAL_STRING("1-1", buf);
    wf_format_month_day(buf, sizeof(buf), 11, 31);
    TEST_ASSERT_EQUAL_STRING("12-31", buf);
}

void test_second_fraction_ms_is_smooth(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_second_fraction_ms(0, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, wf_second_fraction_ms(30, 0));
    /* half a second past 0 -> 0.5s / 60s */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5 / 60.0, wf_second_fraction_ms(0, 500));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 30.25 / 60.0, wf_second_fraction_ms(30, 250));
}

void test_battery_pct_from_mv(void)
{
    TEST_ASSERT_EQUAL_INT(100, wf_battery_pct_from_mv(4200));
    TEST_ASSERT_EQUAL_INT(100, wf_battery_pct_from_mv(4700)); /* on USB: clamp */
    TEST_ASSERT_EQUAL_INT(90, wf_battery_pct_from_mv(4100));
    TEST_ASSERT_EQUAL_INT(95, wf_battery_pct_from_mv(4150));  /* interpolated */
    TEST_ASSERT_EQUAL_INT(50, wf_battery_pct_from_mv(3820));
    TEST_ASSERT_EQUAL_INT(10, wf_battery_pct_from_mv(3680));
    TEST_ASSERT_EQUAL_INT(0, wf_battery_pct_from_mv(3300));
    TEST_ASSERT_EQUAL_INT(0, wf_battery_pct_from_mv(2500));
    /* Monotonic over the whole range. */
    int prev = -1;
    for (int mv = 3000; mv <= 4300; mv += 5) {
        int p = wf_battery_pct_from_mv(mv);
        TEST_ASSERT_TRUE(p >= prev);
        prev = p;
    }
}

void test_battery_bars(void)
{
    TEST_ASSERT_EQUAL_INT(0, wf_battery_bars(-5));
    TEST_ASSERT_EQUAL_INT(0, wf_battery_bars(10));
    TEST_ASSERT_EQUAL_INT(1, wf_battery_bars(11));
    TEST_ASSERT_EQUAL_INT(2, wf_battery_bars(50));
    TEST_ASSERT_EQUAL_INT(3, wf_battery_bars(76));
    TEST_ASSERT_EQUAL_INT(4, wf_battery_bars(86));
    TEST_ASSERT_EQUAL_INT(4, wf_battery_bars(150));
}

void test_battery_level_thresholds(void)
{
    TEST_ASSERT_EQUAL_INT(WF_BATT_OK, wf_battery_level(100));
    TEST_ASSERT_EQUAL_INT(WF_BATT_OK, wf_battery_level(21));
    TEST_ASSERT_EQUAL_INT(WF_BATT_LOW, wf_battery_level(20));
    TEST_ASSERT_EQUAL_INT(WF_BATT_LOW, wf_battery_level(6));
    TEST_ASSERT_EQUAL_INT(WF_BATT_CRIT, wf_battery_level(5));
    TEST_ASSERT_EQUAL_INT(WF_BATT_CRIT, wf_battery_level(0));
    TEST_ASSERT_EQUAL_INT(WF_BATT_CRIT, wf_battery_level(-10)); /* clamp */
    TEST_ASSERT_EQUAL_INT(WF_BATT_OK, wf_battery_level(150));   /* clamp */
}

void test_format_time_boundaries(void)
{
    char b[8];
    wf_format_time(b, sizeof(b), 0, 0);
    TEST_ASSERT_EQUAL_STRING("00:00", b);
    wf_format_time(b, sizeof(b), 23, 59);
    TEST_ASSERT_EQUAL_STRING("23:59", b);
    wf_format_time(b, sizeof(b), 9, 5);
    TEST_ASSERT_EQUAL_STRING("09:05", b);
}

void test_format_date_invalid_month(void)
{
    char b[12];
    wf_format_date(b, sizeof(b), 5, 12);   /* month index out of range */
    TEST_ASSERT_EQUAL_STRING("05 ???", b);
    wf_format_date(b, sizeof(b), 1, -1);
    TEST_ASSERT_EQUAL_STRING("01 ???", b);
}

void test_hour_fraction_12_and_0_are_top(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_hour_fraction(0, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_hour_fraction(12, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0 / 12.0, wf_hour_fraction(13, 0)); /* 1 o'clock */
}

void test_second_fraction_bounds(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_second_fraction(0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, wf_second_fraction(30));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 59.0 / 60.0, wf_second_fraction(59));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_second_fraction);
    RUN_TEST(test_second_fraction_bounds);
    RUN_TEST(test_format_time_boundaries);
    RUN_TEST(test_format_date_invalid_month);
    RUN_TEST(test_hour_fraction_12_and_0_are_top);
    RUN_TEST(test_second_fraction_ms_is_smooth);
    RUN_TEST(test_battery_level_thresholds);
    RUN_TEST(test_battery_pct_from_mv);
    RUN_TEST(test_battery_bars);
    RUN_TEST(test_minute_fraction_is_smooth_with_seconds);
    RUN_TEST(test_hour_fraction_wraps_at_12_and_is_smooth);
    RUN_TEST(test_hand_points_up_at_twelve);
    RUN_TEST(test_hand_points_right_at_quarter);
    RUN_TEST(test_format_time_zero_pads);
    RUN_TEST(test_format_date_uses_month_abbrev);
    RUN_TEST(test_format_steps_groups_thousands);
    RUN_TEST(test_seven_seg_mask_known_digits);
    RUN_TEST(test_seven_seg_mask_rejects_out_of_range);
    RUN_TEST(test_weekday_abbrev);
    RUN_TEST(test_format_month_day_no_leading_zeros);
    return UNITY_END();
}
