#include "unity.h"
#include "pixelstats.h"
#include <string.h>

#define BLACK 0x0000u
#define WHITE 0xFFFFu

void setUp(void) {}
void tearDown(void) {}

void test_is_lit_threshold(void)
{
    TEST_ASSERT_FALSE(wf_pix_is_lit(0, 0, 0, 12));
    TEST_ASSERT_FALSE(wf_pix_is_lit(12, 0, 0, 12));  /* == thresh → not lit */
    TEST_ASSERT_TRUE(wf_pix_is_lit(13, 0, 0, 12));
    TEST_ASSERT_TRUE(wf_pix_is_lit(0, 0, 200, 12));  /* max channel counts  */
}

void test_all_black(void)
{
    uint16_t buf[10 * 10];
    for (int i = 0; i < 100; i++) buf[i] = BLACK;
    wf_pix_stats_t s;
    wf_pix_stats_rgb565(buf, 10, 10, 0, 12, &s);
    TEST_ASSERT_EQUAL_INT(100, s.total);
    TEST_ASSERT_EQUAL_INT(100, s.black);
    TEST_ASSERT_EQUAL_INT(0, s.lit);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, s.black_ratio);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, s.avg_level);
}

void test_all_white(void)
{
    uint16_t buf[10 * 10];
    for (int i = 0; i < 100; i++) buf[i] = WHITE;
    wf_pix_stats_t s;
    wf_pix_stats_rgb565(buf, 10, 10, 0, 12, &s);
    TEST_ASSERT_EQUAL_INT(100, s.lit);
    TEST_ASSERT_EQUAL_INT(0, s.black);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, s.lit_ratio);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, s.avg_level); /* 0xFFFF → 255,255,255 */
}

void test_half_half(void)
{
    uint16_t buf[10 * 10];
    for (int i = 0; i < 100; i++) buf[i] = (i < 50) ? WHITE : BLACK;
    wf_pix_stats_t s;
    wf_pix_stats_rgb565(buf, 10, 10, 0, 12, &s);
    TEST_ASSERT_EQUAL_INT(50, s.black);
    TEST_ASSERT_EQUAL_INT(50, s.lit);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, s.lit_ratio);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, s.avg_level);
}

void test_dim_pixel_counts_black(void)
{
    uint16_t buf[4];
    buf[0] = 0x0001;              /* blue=1 → ~8, <= thresh → black */
    buf[1] = 0x0002;              /* blue=2 → ~16, > thresh → lit   */
    buf[2] = BLACK;
    buf[3] = BLACK;
    wf_pix_stats_t s;
    wf_pix_stats_rgb565(buf, 2, 2, 0, 12, &s);
    TEST_ASSERT_EQUAL_INT(3, s.black);
    TEST_ASSERT_EQUAL_INT(1, s.lit);
}

void test_round_mask_area(void)
{
    static uint16_t buf[100 * 100];
    for (int i = 0; i < 100 * 100; i++) buf[i] = WHITE;
    wf_pix_stats_t s;
    wf_pix_stats_rgb565(buf, 100, 100, 1, 12, &s);
    /* inscribed circle ≈ π/4 of the square */
    TEST_ASSERT_INT_WITHIN(200, (int)(0.785398 * 100 * 100), (int)s.total);
    TEST_ASSERT_TRUE(s.total < 100 * 100);   /* corners excluded */
    TEST_ASSERT_EQUAL_INT(0, s.black);       /* all-white circle */
}

void test_round_mask_excludes_corner(void)
{
    static uint16_t buf[100 * 100];
    for (int i = 0; i < 100 * 100; i++) buf[i] = BLACK;
    buf[0] = WHITE;                          /* top-left corner, outside circle */
    wf_pix_stats_t s;
    wf_pix_stats_rgb565(buf, 100, 100, 1, 12, &s);
    TEST_ASSERT_EQUAL_INT(0, s.lit);         /* the lit corner is not counted */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_is_lit_threshold);
    RUN_TEST(test_all_black);
    RUN_TEST(test_all_white);
    RUN_TEST(test_half_half);
    RUN_TEST(test_dim_pixel_counts_black);
    RUN_TEST(test_round_mask_area);
    RUN_TEST(test_round_mask_excludes_corner);
    return UNITY_END();
}
