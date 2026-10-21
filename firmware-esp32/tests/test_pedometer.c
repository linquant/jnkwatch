#include "unity.h"
#include "pedometer.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Feed a synthetic walk into a pedometer: a static 1g on Z plus a sinusoidal
 * oscillation whose period is one step, so `cadence_hz * seconds` peaks pass.
 * amp_g is the dynamic amplitude (g); fs_hz the sampling rate. Returns the step
 * count. */
static uint32_t walk(ped_t *p, double cadence_hz, double amp_g,
                     double seconds, double fs_hz)
{
    int n = (int)(seconds * fs_hz);
    for (int i = 0; i < n; i++) {
        uint32_t t_ms = (uint32_t)(i * 1000.0 / fs_hz);
        double phase = 2.0 * M_PI * cadence_hz * (t_ms / 1000.0);
        double az = 1.0 + amp_g * sin(phase);
        ped_update(p, 0.0, 0.0, az, t_ms);
    }
    return ped_count(p);
}

/* ---- resting / noise rejection ---- */

void test_rest_counts_no_steps(void)
{
    ped_t p;
    ped_reset(&p);
    /* Still wrist: 1g plus tiny jitter well under the high threshold. */
    uint32_t c = walk(&p, 1.0 /*Hz*/, 0.03 /*g*/, 20.0, 50.0);
    TEST_ASSERT_EQUAL_UINT32(0, c);
}

/* ---- normal walking ---- */

void test_walk_counts_one_step_per_peak(void)
{
    ped_t p;
    ped_reset(&p);
    /* 2 steps/s for 30 s => 60 steps. Allow a small edge/settle tolerance. */
    uint32_t c = walk(&p, 2.0, 0.30, 30.0, 50.0);
    TEST_ASSERT_UINT32_WITHIN(2, 60, c);
}

void test_slow_walk(void)
{
    ped_t p;
    ped_reset(&p);
    /* 1 step/s for 20 s => 20 steps. */
    uint32_t c = walk(&p, 1.0, 0.25, 20.0, 50.0);
    TEST_ASSERT_UINT32_WITHIN(2, 20, c);
}

/* ---- hysteresis: one peak is not double-counted ---- */

void test_no_double_count_on_single_peak(void)
{
    ped_t p;
    ped_reset(&p);
    /* One slow, wide bump (0.5 Hz) over 2 s: a single peak -> a single step. */
    uint32_t c = walk(&p, 0.5, 0.40, 2.0, 50.0);
    TEST_ASSERT_EQUAL_UINT32(1, c);
}

/* ---- refractory caps an implausibly fast cadence ---- */

void test_refractory_caps_cadence(void)
{
    ped_t p;
    ped_reset(&p);
    /* 10 "steps"/s for 2 s would be 20 peaks, but STEP_MIN_MS=200ms caps it
     * at ~5/s, so no more than ~10 are counted. */
    uint32_t c = walk(&p, 10.0, 0.30, 2.0, 100.0);
    TEST_ASSERT_TRUE(c <= 11);
}

/* ---- rate adaptivity: a lower sample rate gives a close count ---- */

void test_rate_adaptive(void)
{
    ped_t fast, slow;
    ped_reset(&fast);
    ped_reset(&slow);
    uint32_t cf = walk(&fast, 2.0, 0.30, 20.0, 50.0);
    uint32_t cs = walk(&slow, 2.0, 0.30, 20.0, 25.0);
    TEST_ASSERT_UINT32_WITHIN(2, cf, cs);
}

/* ---- reset clears the count ---- */

void test_reset_clears_count(void)
{
    ped_t p;
    ped_reset(&p);
    walk(&p, 2.0, 0.30, 10.0, 50.0);
    TEST_ASSERT_TRUE(ped_count(&p) > 0);
    ped_reset(&p);
    TEST_ASSERT_EQUAL_UINT32(0, ped_count(&p));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rest_counts_no_steps);
    RUN_TEST(test_walk_counts_one_step_per_peak);
    RUN_TEST(test_slow_walk);
    RUN_TEST(test_no_double_count_on_single_peak);
    RUN_TEST(test_refractory_caps_cadence);
    RUN_TEST(test_rate_adaptive);
    RUN_TEST(test_reset_clears_count);
    return UNITY_END();
}
