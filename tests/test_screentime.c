#include "unity.h"
#include "screentime.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_init_zero(void)
{
    wf_screentime_t s;
    wf_screentime_init(&s);
    TEST_ASSERT_EQUAL_UINT(0, s.on_ms);
    TEST_ASSERT_EQUAL_UINT(0, s.total_ms);
    TEST_ASSERT_EQUAL_INT(0, wf_screentime_on_pct(&s));
}

void test_accumulates_on_and_total(void)
{
    wf_screentime_t s;
    wf_screentime_init(&s);
    wf_screentime_update(&s, 1000, 1); /* on */
    wf_screentime_update(&s, 500, 0);  /* off */
    wf_screentime_update(&s, 1000, 1); /* on */
    TEST_ASSERT_EQUAL_UINT(2000, s.on_ms);
    TEST_ASSERT_EQUAL_UINT(2500, s.total_ms);
}

void test_on_pct(void)
{
    wf_screentime_t s;
    wf_screentime_init(&s);
    wf_screentime_update(&s, 3000, 1);
    wf_screentime_update(&s, 1000, 0);
    TEST_ASSERT_EQUAL_INT(75, wf_screentime_on_pct(&s)); /* 3000/4000 */
}

void test_format(void)
{
    char b[16];
    wf_screentime_format(b, sizeof(b), 0);        TEST_ASSERT_EQUAL_STRING("0:00:00", b);
    wf_screentime_format(b, sizeof(b), 59000);    TEST_ASSERT_EQUAL_STRING("0:00:59", b);
    wf_screentime_format(b, sizeof(b), 3600000);  TEST_ASSERT_EQUAL_STRING("1:00:00", b);
    wf_screentime_format(b, sizeof(b), 3661000);  TEST_ASSERT_EQUAL_STRING("1:01:01", b);
    wf_screentime_format(b, sizeof(b), 45000000UL); TEST_ASSERT_EQUAL_STRING("12:30:00", b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_zero);
    RUN_TEST(test_accumulates_on_and_total);
    RUN_TEST(test_on_pct);
    RUN_TEST(test_format);
    return UNITY_END();
}
