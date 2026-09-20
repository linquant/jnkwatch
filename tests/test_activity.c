#include "unity.h"
#include "activity.h"

void setUp(void) {}
void tearDown(void) {}

void test_instant_energy(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_activity_instant(0, 0, 1));   /* static */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_activity_instant(0, 1, 0));   /* tilt = no motion */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, wf_activity_instant(0, 0, 2));   /* strong (clamped) */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, wf_activity_instant(0, 0, 1.2)); /* 0.2*2.5 */
}

void test_smoothing_rises_and_decays_within_bounds(void)
{
    wf_activity_t a;
    wf_activity_init(&a);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, a.energy);

    double e = 0;
    for (int i = 0; i < 40; i++) e = wf_activity_update(&a, 0, 0, 2); /* shake */
    TEST_ASSERT_TRUE(e > 0.9 && e <= 1.0);

    for (int i = 0; i < 60; i++) e = wf_activity_update(&a, 0, 0, 1); /* settle */
    TEST_ASSERT_TRUE(e >= 0.0 && e < 0.05);
}

void test_instant_rest_and_tilt_are_zero(void)
{
    /* At rest ‖a‖=1 -> 0; slow tilts keep ‖a‖≈1 so they read 0 too. */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_activity_instant(0, 0, 1));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_activity_instant(0, 1, 0));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_activity_instant(1, 0, 0));
}

void test_instant_clamps_and_midrange(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, wf_activity_instant(0, 0, 2));   /* |1|*gain clamps to 1 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, wf_activity_instant(0, 0, 0));   /* free-fall */
    /* |1.2-1|*2.5 = 0.5 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.5, wf_activity_instant(0, 0, 1.2));
}

void test_update_stays_in_bounds(void)
{
    wf_activity_t a;
    wf_activity_init(&a);
    for (int i = 0; i < 200; i++) {
        double e = wf_activity_update(&a, 0, 0, (i % 2) ? 3.0 : -1.0); /* violent */
        TEST_ASSERT_TRUE(e >= 0.0 && e <= 1.0);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_instant_energy);
    RUN_TEST(test_instant_rest_and_tilt_are_zero);
    RUN_TEST(test_instant_clamps_and_midrange);
    RUN_TEST(test_smoothing_rises_and_decays_within_bounds);
    RUN_TEST(test_update_stays_in_bounds);
    return UNITY_END();
}
