#include "unity.h"
#include "dotfont.h"

void setUp(void) {}
void tearDown(void) {}

void test_space_is_blank(void)
{
    uint8_t r[7];
    wf_dot5x7(' ', r);
    for (int i = 0; i < 7; i++) TEST_ASSERT_EQUAL_HEX8(0x00, r[i]);
}

void test_unknown_char_is_blank(void)
{
    uint8_t r[7];
    wf_dot5x7('#', r);
    for (int i = 0; i < 7; i++) TEST_ASSERT_EQUAL_HEX8(0x00, r[i]);
}

void test_dash_is_only_middle_row(void)
{
    uint8_t r[7];
    wf_dot5x7('-', r);
    TEST_ASSERT_EQUAL_HEX8(0x00, r[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0E, r[3]); /* 01110, centered bar */
    TEST_ASSERT_EQUAL_HEX8(0x00, r[6]);
}

void test_digit_one(void)
{
    uint8_t r[7];
    wf_dot5x7('1', r);
    TEST_ASSERT_EQUAL_HEX8(0x04, r[0]); /* 00100 */
    TEST_ASSERT_EQUAL_HEX8(0x0E, r[6]); /* 01110 base */
}

void test_letter_A_has_crossbar(void)
{
    uint8_t r[7];
    wf_dot5x7('A', r);
    TEST_ASSERT_EQUAL_HEX8(0x0E, r[0]); /* 01110 top */
    TEST_ASSERT_EQUAL_HEX8(0x1F, r[3]); /* 11111 crossbar */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_space_is_blank);
    RUN_TEST(test_unknown_char_is_blank);
    RUN_TEST(test_dash_is_only_middle_row);
    RUN_TEST(test_digit_one);
    RUN_TEST(test_letter_A_has_crossbar);
    return UNITY_END();
}
