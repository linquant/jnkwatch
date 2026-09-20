#include "unity.h"
#include "notify.h"
#include <string.h>
#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

void test_init_empty(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    TEST_ASSERT_EQUAL_INT(0, wf_notif_count(&s));
    TEST_ASSERT_NULL(wf_notif_get(&s, 0));
}

void test_push_newest_first(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, "Messages", "Lea", "Salut", 100);
    wf_notif_push(&s, "Agenda", "Rappel", "Reunion", 200);

    TEST_ASSERT_EQUAL_INT(2, wf_notif_count(&s));
    const wf_notif_t *n0 = wf_notif_get(&s, 0);
    const wf_notif_t *n1 = wf_notif_get(&s, 1);
    TEST_ASSERT_EQUAL_STRING("Agenda", n0->app);   /* newest */
    TEST_ASSERT_EQUAL_STRING("Messages", n1->app); /* older  */
    TEST_ASSERT_NULL(wf_notif_get(&s, 2));
}

void test_unique_increasing_ids(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    unsigned a = wf_notif_push(&s, "A", "t", "b", 1);
    unsigned b = wf_notif_push(&s, "B", "t", "b", 2);
    TEST_ASSERT_NOT_EQUAL(a, b);
    TEST_ASSERT_TRUE(b > a);
    TEST_ASSERT_EQUAL_UINT(b, wf_notif_get(&s, 0)->id);
}

void test_overflow_drops_oldest(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    char t[8];
    for (int i = 0; i < WF_NOTIF_MAX + 3; i++) {
        snprintf(t, sizeof(t), "%d", i);
        wf_notif_push(&s, "app", t, "body", i);
    }
    TEST_ASSERT_EQUAL_INT(WF_NOTIF_MAX, wf_notif_count(&s));
    /* Newest is the last pushed. */
    snprintf(t, sizeof(t), "%d", WF_NOTIF_MAX + 2);
    TEST_ASSERT_EQUAL_STRING(t, wf_notif_get(&s, 0)->title);
    /* Oldest still present is index MAX+2 - (MAX-1). */
    snprintf(t, sizeof(t), "%d", (WF_NOTIF_MAX + 2) - (WF_NOTIF_MAX - 1));
    TEST_ASSERT_EQUAL_STRING(t, wf_notif_get(&s, WF_NOTIF_MAX - 1)->title);
    TEST_ASSERT_NULL(wf_notif_get(&s, WF_NOTIF_MAX));
}

void test_null_fields_become_empty(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, NULL, NULL, NULL, 5);
    const wf_notif_t *n = wf_notif_get(&s, 0);
    TEST_ASSERT_EQUAL_STRING("", n->app);
    TEST_ASSERT_EQUAL_STRING("", n->title);
    TEST_ASSERT_EQUAL_STRING("", n->body);
}

void test_long_fields_truncated_and_terminated(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    char big[300];
    memset(big, 'x', sizeof(big));
    big[sizeof(big) - 1] = '\0';
    wf_notif_push(&s, big, big, big, 1);
    const wf_notif_t *n = wf_notif_get(&s, 0);
    TEST_ASSERT_EQUAL_INT(WF_NOTIF_APP_LEN - 1, (int)strlen(n->app));
    TEST_ASSERT_EQUAL_INT(WF_NOTIF_TITLE_LEN - 1, (int)strlen(n->title));
    TEST_ASSERT_EQUAL_INT(WF_NOTIF_BODY_LEN - 1, (int)strlen(n->body));
}

void test_remove_middle(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, "A", "a", "", 1);
    wf_notif_push(&s, "B", "b", "", 2);
    wf_notif_push(&s, "C", "c", "", 3);   /* recency: C(0), B(1), A(2) */

    int c = wf_notif_remove(&s, 1);       /* drop B */
    TEST_ASSERT_EQUAL_INT(2, c);
    TEST_ASSERT_EQUAL_INT(2, wf_notif_count(&s));
    TEST_ASSERT_EQUAL_STRING("C", wf_notif_get(&s, 0)->app);
    TEST_ASSERT_EQUAL_STRING("A", wf_notif_get(&s, 1)->app);
    TEST_ASSERT_NULL(wf_notif_get(&s, 2));
}

void test_remove_newest(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, "A", "a", "", 1);
    wf_notif_push(&s, "B", "b", "", 2);   /* recency: B(0), A(1) */

    wf_notif_remove(&s, 0);               /* drop the newest, B */
    TEST_ASSERT_EQUAL_INT(1, wf_notif_count(&s));
    TEST_ASSERT_EQUAL_STRING("A", wf_notif_get(&s, 0)->app);
}

void test_remove_out_of_range_noop(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, "A", "a", "", 1);
    TEST_ASSERT_EQUAL_INT(1, wf_notif_remove(&s, 5));
    TEST_ASSERT_EQUAL_INT(1, wf_notif_remove(&s, -1));
    TEST_ASSERT_EQUAL_INT(1, wf_notif_count(&s));
}

void test_remove_until_empty(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, "A", "a", "", 1);
    wf_notif_push(&s, "B", "b", "", 2);
    wf_notif_remove(&s, 0);
    wf_notif_remove(&s, 0);
    TEST_ASSERT_EQUAL_INT(0, wf_notif_count(&s));
    TEST_ASSERT_NULL(wf_notif_get(&s, 0));
    /* store still usable afterwards */
    wf_notif_push(&s, "C", "c", "", 3);
    TEST_ASSERT_EQUAL_STRING("C", wf_notif_get(&s, 0)->app);
}

void test_clear(void)
{
    wf_notif_store_t s;
    wf_notif_init(&s);
    wf_notif_push(&s, "a", "t", "b", 1);
    wf_notif_clear(&s);
    TEST_ASSERT_EQUAL_INT(0, wf_notif_count(&s));
    TEST_ASSERT_NULL(wf_notif_get(&s, 0));
    /* ids keep increasing after a clear */
    unsigned id2 = wf_notif_push(&s, "a", "t", "b", 2);
    TEST_ASSERT_TRUE(id2 > 0);
}

void test_reltime_buckets(void)
{
    char b[16];
    long now = 1000000;
    wf_notif_reltime(b, sizeof(b), now, now);        TEST_ASSERT_EQUAL_STRING("now", b);
    wf_notif_reltime(b, sizeof(b), now, now - 30);   TEST_ASSERT_EQUAL_STRING("now", b);
    wf_notif_reltime(b, sizeof(b), now, now - 60);   TEST_ASSERT_EQUAL_STRING("1m", b);
    wf_notif_reltime(b, sizeof(b), now, now - 5*60); TEST_ASSERT_EQUAL_STRING("5m", b);
    wf_notif_reltime(b, sizeof(b), now, now - 3600);      TEST_ASSERT_EQUAL_STRING("1h", b);
    wf_notif_reltime(b, sizeof(b), now, now - 5*3600);    TEST_ASSERT_EQUAL_STRING("5h", b);
    wf_notif_reltime(b, sizeof(b), now, now - 86400);     TEST_ASSERT_EQUAL_STRING("1j", b);
    wf_notif_reltime(b, sizeof(b), now, now - 3*86400);   TEST_ASSERT_EQUAL_STRING("3j", b);
    /* future timestamp clamps to "now" */
    wf_notif_reltime(b, sizeof(b), now, now + 500);  TEST_ASSERT_EQUAL_STRING("now", b);
}

void test_reltime_bucket_edges(void)
{
    char b[16];
    long now = 1000000;
    wf_notif_reltime(b, sizeof(b), now, now - 59);      TEST_ASSERT_EQUAL_STRING("now", b);
    wf_notif_reltime(b, sizeof(b), now, now - 3599);    TEST_ASSERT_EQUAL_STRING("59m", b);
    wf_notif_reltime(b, sizeof(b), now, now - 86399);   TEST_ASSERT_EQUAL_STRING("23h", b);
    wf_notif_reltime(b, sizeof(b), now, now - 86400);   TEST_ASSERT_EQUAL_STRING("1j", b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_empty);
    RUN_TEST(test_push_newest_first);
    RUN_TEST(test_unique_increasing_ids);
    RUN_TEST(test_overflow_drops_oldest);
    RUN_TEST(test_null_fields_become_empty);
    RUN_TEST(test_long_fields_truncated_and_terminated);
    RUN_TEST(test_remove_middle);
    RUN_TEST(test_remove_newest);
    RUN_TEST(test_remove_out_of_range_noop);
    RUN_TEST(test_remove_until_empty);
    RUN_TEST(test_clear);
    RUN_TEST(test_reltime_buckets);
    RUN_TEST(test_reltime_bucket_edges);
    return UNITY_END();
}
