#include "unity.h"
#include "actions.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_init_seeds_defaults(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    TEST_ASSERT_EQUAL_INT(6, wf_actions_count(&s));
    const wf_action_t *a = wf_actions_get(&s, 0);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL_UINT(1, a->id);
    TEST_ASSERT_EQUAL_STRING("btn1", a->label);
    TEST_ASSERT_EQUAL_INT(WF_ICON_GENERIC, a->icon);
    TEST_ASSERT_TRUE(a->enabled);
}

void test_get_out_of_range_null(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    TEST_ASSERT_NOT_NULL(wf_actions_get(&s, 0));
    TEST_ASSERT_NULL(wf_actions_get(&s, -1));
    TEST_ASSERT_NULL(wf_actions_get(&s, wf_actions_count(&s)));
    TEST_ASSERT_NULL(wf_actions_get(&s, WF_ACTION_MAX));
}

void test_add_appends(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    wf_actions_clear(&s);
    int i = wf_actions_add(&s, 42, "Cafe", WF_ICON_HOME, true);
    TEST_ASSERT_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, wf_actions_count(&s));
    const wf_action_t *a = wf_actions_get(&s, 0);
    TEST_ASSERT_EQUAL_UINT(42, a->id);
    TEST_ASSERT_EQUAL_STRING("Cafe", a->label);
    TEST_ASSERT_EQUAL_INT(WF_ICON_HOME, a->icon);
}

void test_add_full_returns_error(void)
{
    wf_actions_t s;
    wf_actions_clear(&s);
    for (int i = 0; i < WF_ACTION_MAX; i++)
        TEST_ASSERT_EQUAL_INT(i, wf_actions_add(&s, (unsigned)i, "x", WF_ICON_NONE, true));
    TEST_ASSERT_EQUAL_INT(-1, wf_actions_add(&s, 99, "y", WF_ICON_NONE, true));
    TEST_ASSERT_EQUAL_INT(WF_ACTION_MAX, wf_actions_count(&s));
}

void test_add_truncates_label(void)
{
    wf_actions_t s;
    wf_actions_clear(&s);
    char big[64];
    memset(big, 'x', sizeof(big));
    big[sizeof(big) - 1] = '\0';
    wf_actions_add(&s, 1, big, WF_ICON_NONE, true);
    TEST_ASSERT_EQUAL_INT(WF_ACTION_LABEL_LEN - 1, (int)strlen(wf_actions_get(&s, 0)->label));
}

void test_add_null_label_becomes_empty(void)
{
    wf_actions_t s;
    wf_actions_clear(&s);
    wf_actions_add(&s, 1, NULL, WF_ICON_NONE, true);
    TEST_ASSERT_EQUAL_STRING("", wf_actions_get(&s, 0)->label);
}

void test_add_clamps_invalid_icon(void)
{
    wf_actions_t s;
    wf_actions_clear(&s);
    wf_actions_add(&s, 1, "a", WF_ICON_COUNT, true);
    wf_actions_add(&s, 2, "b", (wf_icon_t)(-3), true);
    TEST_ASSERT_EQUAL_INT(WF_ICON_NONE, wf_actions_get(&s, 0)->icon);
    TEST_ASSERT_EQUAL_INT(WF_ICON_NONE, wf_actions_get(&s, 1)->icon);
}

void test_set_replaces_in_place(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    int c = wf_actions_count(&s);
    int r = wf_actions_set(&s, 1, 77, "Zap", WF_ICON_BELL, false);
    TEST_ASSERT_EQUAL_INT(1, r);
    TEST_ASSERT_EQUAL_INT(c, wf_actions_count(&s));       /* count unchanged */
    const wf_action_t *a = wf_actions_get(&s, 1);
    TEST_ASSERT_EQUAL_UINT(77, a->id);
    TEST_ASSERT_EQUAL_STRING("Zap", a->label);
    TEST_ASSERT_EQUAL_INT(WF_ICON_BELL, a->icon);
    TEST_ASSERT_FALSE(a->enabled);
    /* neighbour intact */
    TEST_ASSERT_EQUAL_UINT(1, wf_actions_get(&s, 0)->id);
}

void test_set_out_of_range_noop(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    int c = wf_actions_count(&s);
    TEST_ASSERT_EQUAL_INT(-1, wf_actions_set(&s, -1, 1, "a", WF_ICON_NONE, true));
    TEST_ASSERT_EQUAL_INT(-1, wf_actions_set(&s, c, 1, "a", WF_ICON_NONE, true));
    TEST_ASSERT_EQUAL_INT(c, wf_actions_count(&s));
}

void test_set_enabled_toggles(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    wf_actions_set_enabled(&s, 0, false);
    TEST_ASSERT_FALSE(wf_actions_get(&s, 0)->enabled);
    wf_actions_set_enabled(&s, 0, true);
    TEST_ASSERT_TRUE(wf_actions_get(&s, 0)->enabled);
    wf_actions_set_enabled(&s, 999, true); /* no crash / no-op */
}

void test_clear_then_reusable(void)
{
    wf_actions_t s;
    wf_actions_init(&s);
    wf_actions_clear(&s);
    TEST_ASSERT_EQUAL_INT(0, wf_actions_count(&s));
    TEST_ASSERT_NULL(wf_actions_get(&s, 0));
    /* BLE resync path: rebuild after clear */
    wf_actions_add(&s, 5, "New", WF_ICON_MUSIC, true);
    TEST_ASSERT_EQUAL_INT(1, wf_actions_count(&s));
    TEST_ASSERT_EQUAL_STRING("New", wf_actions_get(&s, 0)->label);
}

void test_page_count(void)
{
    wf_actions_t s;
    wf_actions_clear(&s);
    TEST_ASSERT_EQUAL_INT(1, wf_actions_page_count(&s));          /* 0 -> 1 */
    for (int i = 0; i < WF_ACTIONS_PER_PAGE; i++)
        wf_actions_add(&s, (unsigned)i, "x", WF_ICON_NONE, true);
    TEST_ASSERT_EQUAL_INT(1, wf_actions_page_count(&s));          /* 6 -> 1 */
    wf_actions_add(&s, 99, "y", WF_ICON_NONE, true);
    TEST_ASSERT_EQUAL_INT(2, wf_actions_page_count(&s));          /* 7 -> 2 */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_seeds_defaults);
    RUN_TEST(test_get_out_of_range_null);
    RUN_TEST(test_add_appends);
    RUN_TEST(test_add_full_returns_error);
    RUN_TEST(test_add_truncates_label);
    RUN_TEST(test_add_null_label_becomes_empty);
    RUN_TEST(test_add_clamps_invalid_icon);
    RUN_TEST(test_set_replaces_in_place);
    RUN_TEST(test_set_out_of_range_noop);
    RUN_TEST(test_set_enabled_toggles);
    RUN_TEST(test_clear_then_reusable);
    RUN_TEST(test_page_count);
    return UNITY_END();
}
