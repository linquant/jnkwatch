#include "unity.h"
#include "probes.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_init_empty(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    TEST_ASSERT_EQUAL_INT(0, wf_probes_count(&p));
    TEST_ASSERT_FALSE(p.slot[0].used);
    TEST_ASSERT_EQUAL_STRING("", p.slot[0].name);
}

void test_set_and_count(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, 0, "RDC", "21.67", 1000);
    wf_probes_set(&p, 2, "Garage", "8", 2000);
    TEST_ASSERT_EQUAL_INT(2, wf_probes_count(&p));
    TEST_ASSERT_TRUE(p.slot[0].used);
    TEST_ASSERT_EQUAL_STRING("RDC", p.slot[0].name);
    TEST_ASSERT_EQUAL_STRING("21.67", p.slot[0].value);
    TEST_ASSERT_EQUAL_UINT32(1000, p.slot[0].updated_ms);
    TEST_ASSERT_FALSE(p.slot[1].used);
    TEST_ASSERT_EQUAL_STRING("Garage", p.slot[2].name);
}

void test_set_refreshes_same_slot(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, 0, "RDC", "21.0", 1000);
    wf_probes_set(&p, 0, "RDC", "22.5", 5000);
    TEST_ASSERT_EQUAL_INT(1, wf_probes_count(&p));
    TEST_ASSERT_EQUAL_STRING("22.5", p.slot[0].value);
    TEST_ASSERT_EQUAL_UINT32(5000, p.slot[0].updated_ms);
}

void test_set_rejects_out_of_range(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, -1, "X", "1", 10);
    wf_probes_set(&p, WF_PROBE_MAX, "X", "1", 10);
    TEST_ASSERT_EQUAL_INT(0, wf_probes_count(&p));
}

void test_set_rejects_empty_name(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, 0, "", "1", 10);
    wf_probes_set(&p, 0, NULL, "1", 10);
    TEST_ASSERT_EQUAL_INT(0, wf_probes_count(&p));
}

void test_null_value_stored_empty(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, 0, "RDC", NULL, 10);
    TEST_ASSERT_TRUE(p.slot[0].used);
    TEST_ASSERT_EQUAL_STRING("", p.slot[0].value);
}

void test_clear(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, 0, "RDC", "21", 10);
    wf_probes_clear(&p, 0);
    TEST_ASSERT_EQUAL_INT(0, wf_probes_count(&p));
    TEST_ASSERT_FALSE(p.slot[0].used);
}

void test_name_and_value_truncated(void)
{
    wf_probes_t p;
    wf_probes_init(&p);
    wf_probes_set(&p, 0, "AReallyLongProbeNameThatOverflowsTheBuffer",
                  "AReallyLongProbeValueThatAlsoOverflowsThisBufferHere123456", 10);
    TEST_ASSERT_TRUE(strlen(p.slot[0].name) < WF_PROBE_NAME_LEN);
    TEST_ASSERT_EQUAL_CHAR('\0', p.slot[0].name[WF_PROBE_NAME_LEN - 1]);
    TEST_ASSERT_TRUE(strlen(p.slot[0].value) < WF_PROBE_VALUE_LEN);
    TEST_ASSERT_EQUAL_CHAR('\0', p.slot[0].value[WF_PROBE_VALUE_LEN - 1]);
}

void test_format_age(void)
{
    char buf[32];
    wf_probes_format_age(buf, sizeof buf, 10000, 10000);      /* 0 s   */
    TEST_ASSERT_EQUAL_STRING("a l'instant", buf);
    wf_probes_format_age(buf, sizeof buf, 10000, 19000);      /* 9 s   */
    TEST_ASSERT_EQUAL_STRING("a l'instant", buf);
    wf_probes_format_age(buf, sizeof buf, 10000, 25000);      /* 15 s  */
    TEST_ASSERT_EQUAL_STRING("il y a 15 s", buf);
    wf_probes_format_age(buf, sizeof buf, 0, 300000);         /* 5 min */
    TEST_ASSERT_EQUAL_STRING("il y a 5 min", buf);
    wf_probes_format_age(buf, sizeof buf, 0, 7200000);        /* 2 h   */
    TEST_ASSERT_EQUAL_STRING("il y a 2 h", buf);
    wf_probes_format_age(buf, sizeof buf, 0, 3UL * 86400000UL); /* 3 j */
    TEST_ASSERT_EQUAL_STRING("il y a 3 j", buf);
}

void test_format_age_clock_went_back(void)
{
    char buf[32];
    /* now < updated (RTC resync): must not underflow to a huge age. */
    wf_probes_format_age(buf, sizeof buf, 50000, 10000);
    TEST_ASSERT_EQUAL_STRING("a l'instant", buf);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_empty);
    RUN_TEST(test_set_and_count);
    RUN_TEST(test_set_refreshes_same_slot);
    RUN_TEST(test_set_rejects_out_of_range);
    RUN_TEST(test_set_rejects_empty_name);
    RUN_TEST(test_null_value_stored_empty);
    RUN_TEST(test_clear);
    RUN_TEST(test_name_and_value_truncated);
    RUN_TEST(test_format_age);
    RUN_TEST(test_format_age_clock_went_back);
    return UNITY_END();
}
