#include "unity.h"
#include "timer.h"

void setUp(void) {}
void tearDown(void) {}

void test_init_sets_idle_and_clamps(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 5 * 60 * 1000);
    TEST_ASSERT_EQUAL_INT(WF_TMR_IDLE, t.state);
    TEST_ASSERT_EQUAL_INT(5 * 60 * 1000, t.duration_ms);
    TEST_ASSERT_EQUAL_INT(5 * 60 * 1000, t.remaining_ms);

    wf_timer_init(&t, 10);                 /* below min */
    TEST_ASSERT_EQUAL_INT(WF_TMR_MIN_MS, t.duration_ms);
    wf_timer_init(&t, 999 * 60 * 1000);    /* above max */
    TEST_ASSERT_EQUAL_INT(WF_TMR_MAX_MS, t.duration_ms);
}

void test_set_duration_idle_only(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 5 * 60 * 1000);
    wf_timer_set_duration(&t, 3 * 60 * 1000);
    TEST_ASSERT_EQUAL_INT(3 * 60 * 1000, t.duration_ms);
    TEST_ASSERT_EQUAL_INT(3 * 60 * 1000, t.remaining_ms);

    wf_timer_start(&t);                    /* now RUNNING */
    wf_timer_set_duration(&t, 9 * 60 * 1000);
    TEST_ASSERT_EQUAL_INT(3 * 60 * 1000, t.duration_ms); /* ignored */
}

void test_countdown_and_done(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 60 * 1000);
    wf_timer_start(&t);
    TEST_ASSERT_EQUAL_INT(WF_TMR_RUNNING, t.state);
    wf_timer_tick(&t, 20 * 1000);
    TEST_ASSERT_EQUAL_INT(40 * 1000, t.remaining_ms);
    wf_timer_tick(&t, 50 * 1000);          /* overshoot */
    TEST_ASSERT_EQUAL_INT(0, t.remaining_ms);
    TEST_ASSERT_EQUAL_INT(WF_TMR_DONE, t.state);
    wf_timer_tick(&t, 1000);               /* no effect when not running */
    TEST_ASSERT_EQUAL_INT(0, t.remaining_ms);
}

void test_pause_resume_and_reset(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 60 * 1000);
    wf_timer_start(&t);
    wf_timer_tick(&t, 15 * 1000);
    wf_timer_pause(&t);
    TEST_ASSERT_EQUAL_INT(WF_TMR_PAUSED, t.state);
    wf_timer_tick(&t, 10 * 1000);          /* paused: no change */
    TEST_ASSERT_EQUAL_INT(45 * 1000, t.remaining_ms);
    wf_timer_start(&t);
    TEST_ASSERT_EQUAL_INT(WF_TMR_RUNNING, t.state);
    wf_timer_reset(&t);
    TEST_ASSERT_EQUAL_INT(WF_TMR_IDLE, t.state);
    TEST_ASSERT_EQUAL_INT(60 * 1000, t.remaining_ms);
}

void test_toggle(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 60 * 1000);
    wf_timer_toggle(&t);                   /* idle -> running */
    TEST_ASSERT_EQUAL_INT(WF_TMR_RUNNING, t.state);
    wf_timer_toggle(&t);                   /* running -> paused */
    TEST_ASSERT_EQUAL_INT(WF_TMR_PAUSED, t.state);
    wf_timer_toggle(&t);                   /* paused -> running */
    TEST_ASSERT_EQUAL_INT(WF_TMR_RUNNING, t.state);
}

void test_remaining_fraction(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 60 * 1000);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, wf_timer_remaining_fraction(&t));
    wf_timer_start(&t);
    wf_timer_tick(&t, 45 * 1000);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, wf_timer_remaining_fraction(&t));
}

void test_format_rounds_up(void)
{
    wf_timer_t t;
    char b[8];
    wf_timer_init(&t, 5 * 60 * 1000);
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("05:00", b);
    wf_timer_start(&t);
    wf_timer_tick(&t, 500);                /* 299500ms -> ceil 300s */
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("05:00", b);
    wf_timer_tick(&t, 299500 - 1000 + 1);  /* leave ~999ms */
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("00:01", b);
    wf_timer_tick(&t, 5000);               /* done */
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("00:00", b);
}

void test_tick_ignored_unless_running(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 5 * 60 * 1000);
    wf_timer_tick(&t, 10000);                 /* IDLE: no effect */
    TEST_ASSERT_EQUAL_INT(5 * 60 * 1000, t.remaining_ms);
    wf_timer_start(&t);
    wf_timer_pause(&t);
    int before = t.remaining_ms;
    wf_timer_tick(&t, 10000);                 /* PAUSED: no effect */
    TEST_ASSERT_EQUAL_INT(before, t.remaining_ms);
}

void test_toggle_on_done_is_noop(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 60 * 1000);
    wf_timer_start(&t);
    wf_timer_tick(&t, 60 * 1000);             /* -> DONE */
    TEST_ASSERT_EQUAL_INT(WF_TMR_DONE, t.state);
    wf_timer_toggle(&t);                      /* stays DONE */
    TEST_ASSERT_EQUAL_INT(WF_TMR_DONE, t.state);
    TEST_ASSERT_EQUAL_INT(0, t.remaining_ms);
}

void test_set_duration_ignored_when_running(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 5 * 60 * 1000);
    wf_timer_start(&t);
    wf_timer_set_duration(&t, 10 * 60 * 1000);
    TEST_ASSERT_EQUAL_INT(5 * 60 * 1000, t.duration_ms); /* unchanged */
}

void test_format_long_and_clamps(void)
{
    char b[8];
    wf_timer_t t;
    wf_timer_init(&t, 25 * 60 * 1000);
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("25:00", b);

    wf_timer_init(&t, 200 * 60 * 1000);       /* over max -> 99 min */
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("99:00", b);

    wf_timer_init(&t, 0);                     /* under min -> 1 min */
    wf_timer_format(&t, b, sizeof(b));
    TEST_ASSERT_EQUAL_STRING("01:00", b);
}

void test_remaining_fraction_full_half_empty(void)
{
    wf_timer_t t;
    wf_timer_init(&t, 4 * 60 * 1000);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, wf_timer_remaining_fraction(&t));
    wf_timer_start(&t);
    wf_timer_tick(&t, 2 * 60 * 1000);         /* half */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, wf_timer_remaining_fraction(&t));
    wf_timer_tick(&t, 5 * 60 * 1000);         /* past end -> 0 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, wf_timer_remaining_fraction(&t));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_idle_and_clamps);
    RUN_TEST(test_set_duration_idle_only);
    RUN_TEST(test_set_duration_ignored_when_running);
    RUN_TEST(test_countdown_and_done);
    RUN_TEST(test_tick_ignored_unless_running);
    RUN_TEST(test_pause_resume_and_reset);
    RUN_TEST(test_toggle);
    RUN_TEST(test_toggle_on_done_is_noop);
    RUN_TEST(test_remaining_fraction);
    RUN_TEST(test_remaining_fraction_full_half_empty);
    RUN_TEST(test_format_rounds_up);
    RUN_TEST(test_format_long_and_clamps);
    return UNITY_END();
}
