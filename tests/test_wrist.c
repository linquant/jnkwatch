#include "unity.h"
#include "wrist.h"

void setUp(void) {}
void tearDown(void) {}

void test_tilt_deg(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 0.0,   wf_wrist_tilt_deg(0, 0, 1));   /* face up */
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 90.0,  wf_wrist_tilt_deg(0, 1, 0));   /* vertical */
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 180.0, wf_wrist_tilt_deg(0, 0, -1));  /* face down */
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 45.0,  wf_wrist_tilt_deg(0, 0.707, 0.707));
}

void test_wake_on_rest_to_view_edge(void)
{
    wf_wrist_t w;
    wf_wrist_init(&w);
    /* resting (arm down, ~90deg): no wake */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 1, 0));
    /* raise to viewing (~37deg): fires once */
    TEST_ASSERT_TRUE(wf_wrist_update(&w, 0, 0.6, 0.8));
    /* still viewing: does not re-fire */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.6, 0.8));
    /* back to resting: no wake */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 1, 0));
    /* raise again: fires again */
    TEST_ASSERT_TRUE(wf_wrist_update(&w, 0, 0.6, 0.8));
}

void test_hysteresis_midband_is_stable(void)
{
    wf_wrist_t w;
    wf_wrist_init(&w);
    /* tilt ~65deg (between the two thresholds) from rest: stays not-raised */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.906, 0.423)); /* ~65deg */
    /* now genuinely raise */
    TEST_ASSERT_TRUE(wf_wrist_update(&w, 0, 0.5, 0.87));     /* ~30deg */
    /* drift back to ~65deg: stays raised (no re-fire, no reset) */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.906, 0.423));
    /* a later raise should NOT fire because we never dropped to resting */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.5, 0.87));
}

void test_sensitivity_changes_threshold(void)
{
    wf_wrist_t w;
    wf_wrist_init(&w);                      /* Moyenne: up_th = 55deg */
    /* ~60deg raise: not flat enough to wake at Moyenne */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.866, 0.5));

    /* Haute (up_th = 65deg): the same ~60deg raise now fires */
    wf_wrist_init(&w);
    wf_wrist_set_sensitivity(&w, 2);
    TEST_ASSERT_TRUE(wf_wrist_update(&w, 0, 0.866, 0.5));

    /* Basse (up_th = 45deg): a ~50deg raise is not enough */
    wf_wrist_init(&w);
    wf_wrist_set_sensitivity(&w, 0);
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.766, 0.643));
    /* but a ~30deg raise still wakes it */
    TEST_ASSERT_TRUE(wf_wrist_update(&w, 0, 0.5, 0.866));
}

void test_sensitivity_clamps(void)
{
    wf_wrist_t w;
    wf_wrist_init(&w);
    /* out-of-range levels clamp to the nearest valid level (no crash / UB) */
    wf_wrist_set_sensitivity(&w, 99);   /* -> Haute */
    TEST_ASSERT_TRUE(wf_wrist_update(&w, 0, 0.866, 0.5));  /* ~60deg wakes at Haute */
    wf_wrist_init(&w);
    wf_wrist_set_sensitivity(&w, -5);   /* -> Basse */
    TEST_ASSERT_FALSE(wf_wrist_update(&w, 0, 0.766, 0.643)); /* ~50deg: no wake at Basse */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tilt_deg);
    RUN_TEST(test_wake_on_rest_to_view_edge);
    RUN_TEST(test_hysteresis_midband_is_stable);
    RUN_TEST(test_sensitivity_changes_threshold);
    RUN_TEST(test_sensitivity_clamps);
    return UNITY_END();
}
