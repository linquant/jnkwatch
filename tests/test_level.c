#include "unity.h"
#include "level.h"
#include <math.h>

/* sin/cos of 30 degrees, to build tilt vectors without needing M_PI. */
#define S30 0.5
#define C30 0.86602540378

void setUp(void) {}
void tearDown(void) {}

void test_flat_is_level(void)
{
    wf_level_t l;
    wf_level_compute(0.0, 0.0, 1.0, &l);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.roll_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.pitch_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.tilt_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.bx);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.by);
    TEST_ASSERT_TRUE(l.level);
}

void test_roll_right_30(void)
{
    wf_level_t l;
    wf_level_compute(S30, 0.0, C30, &l);           /* right edge down 30deg */
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 30.0, l.roll_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 0.0,  l.pitch_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 30.0, l.tilt_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.5, l.bx);     /* ball rolls to low (+X) side */
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.by);
    TEST_ASSERT_FALSE(l.level);
}

void test_pitch_forward_30(void)
{
    wf_level_t l;
    wf_level_compute(0.0, S30, C30, &l);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 30.0, l.pitch_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 0.0,  l.roll_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.5, l.by);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.bx);
}

void test_roll_sign_is_left(void)
{
    wf_level_t l;
    wf_level_compute(-S30, 0.0, C30, &l);           /* left edge down */
    TEST_ASSERT_DOUBLE_WITHIN(0.1, -30.0, l.roll_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -0.5, l.bx);
}

void test_tilt_extremes(void)
{
    wf_level_t l;
    wf_level_compute(0.0, 1.0, 0.0, &l);            /* vertical */
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 90.0, l.tilt_deg);
    wf_level_compute(0.0, 0.0, -1.0, &l);           /* upside down */
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 180.0, l.tilt_deg);
    TEST_ASSERT_FALSE(l.level);
}

void test_ball_stays_in_unit_disk(void)
{
    wf_level_t l;
    wf_level_compute(1.0, 0.0, 0.0, &l);            /* fully on the +X side */
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, l.bx);
    TEST_ASSERT_TRUE(l.bx * l.bx + l.by * l.by <= 1.0 + 1e-9);

    wf_level_compute(3.0, 4.0, 0.0, &l);            /* mag 5 -> normalized 0.6/0.8 */
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.6, l.bx);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.8, l.by);
}

void test_level_tolerance_band(void)
{
    wf_level_t l;
    double small = 0.02;                            /* ~1.1deg -> level */
    wf_level_compute(small, 0.0, sqrt(1.0 - small * small), &l);
    TEST_ASSERT_TRUE(l.level);

    double big = 0.1;                               /* ~5.7deg -> not level */
    wf_level_compute(big, 0.0, sqrt(1.0 - big * big), &l);
    TEST_ASSERT_FALSE(l.level);
}

void test_zero_vector_is_safe(void)
{
    wf_level_t l;
    wf_level_compute(0.0, 0.0, 0.0, &l);            /* no div-by-zero, not level */
    TEST_ASSERT_FALSE(l.level);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.bx);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.by);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, l.tilt_deg);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flat_is_level);
    RUN_TEST(test_roll_right_30);
    RUN_TEST(test_pitch_forward_30);
    RUN_TEST(test_roll_sign_is_left);
    RUN_TEST(test_tilt_extremes);
    RUN_TEST(test_ball_stays_in_unit_disk);
    RUN_TEST(test_level_tolerance_band);
    RUN_TEST(test_zero_vector_is_safe);
    return UNITY_END();
}
