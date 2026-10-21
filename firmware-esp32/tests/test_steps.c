#include "unity.h"
#include "steps.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Feed a synthetic 2 Hz walk for `seconds` at 50 Hz, on day `yday`, advancing a
 * shared millisecond clock across calls. */
static void walk(wf_steps_t *s, double seconds, int yday, uint32_t *clock_ms)
{
    int n = (int)(seconds * 50.0);
    for (int i = 0; i < n; i++) {
        double t = *clock_ms / 1000.0;
        double az = 1.0 + 0.30 * sin(2.0 * M_PI * 2.0 * t);
        wf_steps_update(s, 0.0, 0.0, az, *clock_ms, yday);
        *clock_ms += 20;
    }
}

void test_accumulates_today(void)
{
    wf_steps_t s;
    wf_steps_init(&s);
    uint32_t clk = 0;
    walk(&s, 30.0, 100, &clk);          /* 2/s * 30 s ~= 60 steps */
    TEST_ASSERT_UINT32_WITHIN(2, 60, wf_steps_today(&s));
    TEST_ASSERT_EQUAL_INT(100, wf_steps_yday(&s));
}

void test_midnight_resets(void)
{
    wf_steps_t s;
    wf_steps_init(&s);
    uint32_t clk = 0;
    walk(&s, 20.0, 100, &clk);
    TEST_ASSERT_TRUE(wf_steps_today(&s) > 0);
    /* New day: the next sample rolls the total back to 0, then counts afresh. */
    walk(&s, 20.0, 101, &clk);
    TEST_ASSERT_UINT32_WITHIN(2, 40, wf_steps_today(&s)); /* ~40 from day 101 only */
    TEST_ASSERT_EQUAL_INT(101, wf_steps_yday(&s));
}

void test_year_wrap_resets(void)
{
    wf_steps_t s;
    wf_steps_init(&s);
    uint32_t clk = 0;
    walk(&s, 20.0, 364, &clk);     /* Dec 31 */
    uint32_t before = wf_steps_today(&s);
    TEST_ASSERT_TRUE(before > 0);
    walk(&s, 10.0, 0, &clk);       /* Jan 1 -> yday 0 differs -> reset */
    TEST_ASSERT_TRUE(wf_steps_today(&s) < before);
    TEST_ASSERT_EQUAL_INT(0, wf_steps_yday(&s));
}

void test_restore_keeps_total_same_day(void)
{
    wf_steps_t s;
    wf_steps_restore(&s, 1234, 100);
    TEST_ASSERT_EQUAL_UINT32(1234, wf_steps_today(&s));
    uint32_t clk = 0;
    walk(&s, 10.0, 100, &clk);     /* same day -> adds on top, no reset */
    TEST_ASSERT_TRUE(wf_steps_today(&s) > 1234);
}

void test_restore_old_day_resets_on_update(void)
{
    wf_steps_t s;
    wf_steps_restore(&s, 5000, 99); /* yesterday */
    uint32_t clk = 0;
    walk(&s, 10.0, 100, &clk);      /* today differs -> reset then count */
    TEST_ASSERT_UINT32_WITHIN(2, 20, wf_steps_today(&s));
    TEST_ASSERT_EQUAL_INT(100, wf_steps_yday(&s));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_accumulates_today);
    RUN_TEST(test_midnight_resets);
    RUN_TEST(test_year_wrap_resets);
    RUN_TEST(test_restore_keeps_total_same_day);
    RUN_TEST(test_restore_old_day_resets_on_update);
    return UNITY_END();
}
