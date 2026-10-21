#include "unity.h"
#include "ota_proto.h"
#include "sha256.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

#define SLOT_CAP (4u * 1024 * 1024)   /* ota_0/ota_1 size from partitions.csv */

/* --- helpers --------------------------------------------------------------- */
static void sha_of(const uint8_t *data, size_t len, uint8_t out[WF_OTA_SHA_LEN])
{
    wf_sha256_t c;
    wf_sha256_init(&c);
    wf_sha256_update(&c, data, len);
    wf_sha256_final(&c, out);
}

static int build_start(uint8_t *buf, uint32_t size, uint16_t chunk,
                       const uint8_t sha[WF_OTA_SHA_LEN])
{
    buf[0] = WF_OTA_OP_START;
    buf[1] = (uint8_t)size;         buf[2] = (uint8_t)(size >> 8);
    buf[3] = (uint8_t)(size >> 16); buf[4] = (uint8_t)(size >> 24);
    buf[5] = (uint8_t)chunk;        buf[6] = (uint8_t)(chunk >> 8);
    memcpy(buf + 7, sha, WF_OTA_SHA_LEN);
    return WF_OTA_START_LEN;
}

/* Run a whole transfer of `img`; returns the commit result and fills *st. */
static int run_transfer(const uint8_t *img, uint32_t size, uint16_t req_chunk,
                        const uint8_t sha[WF_OTA_SHA_LEN], wf_ota_t *o)
{
    uint8_t start[WF_OTA_START_LEN];
    uint8_t err = 0xFF;
    build_start(start, size, req_chunk, sha);
    int chunk = wf_ota_begin(o, start, WF_OTA_START_LEN, SLOT_CAP, WF_OTA_MAX_CHUNK, &err);
    if (chunk < 0) return -100;

    uint8_t frame[2 + WF_OTA_MAX_CHUNK];
    uint16_t seq = 0;
    uint32_t off = 0;
    while (off < size) {
        uint32_t n = size - off;
        if (n > (uint32_t)chunk) n = (uint32_t)chunk;
        frame[0] = (uint8_t)seq; frame[1] = (uint8_t)(seq >> 8);
        memcpy(frame + 2, img + off, n);
        if (wf_ota_feed(o, frame, (int)(2 + n), &err) < 0) return -200;
        off += n; seq++;
    }
    return wf_ota_commit(o, &err);
}

/* --- tests ----------------------------------------------------------------- */

void test_sha256_abc_vector(void)
{
    /* FIPS 180-4 example: SHA-256("abc"). */
    static const uint8_t expect[32] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };
    uint8_t out[32];
    sha_of((const uint8_t *)"abc", 3, out);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, out, 32);
}

void test_full_transfer_ok(void)
{
    uint8_t img[1000];
    for (int i = 0; i < 1000; i++) img[i] = (uint8_t)(i * 7 + 3);
    uint8_t sha[32]; sha_of(img, sizeof(img), sha);

    wf_ota_t o; wf_ota_init(&o);
    TEST_ASSERT_EQUAL_INT(0, run_transfer(img, sizeof(img), 244, sha, &o));
    TEST_ASSERT_EQUAL_INT(WF_OTA_DONE, o.state);
    TEST_ASSERT_EQUAL_UINT32(1000, o.received);
}

void test_chunk_clamp(void)
{
    uint8_t sha[32] = {0};
    wf_ota_t o; wf_ota_init(&o);
    uint8_t start[WF_OTA_START_LEN]; uint8_t err = 0xFF;

    /* Oversized request clamps down to WF_OTA_MAX_CHUNK. */
    build_start(start, 500, 60000, sha);
    TEST_ASSERT_EQUAL_INT(WF_OTA_MAX_CHUNK,
        wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, WF_OTA_MAX_CHUNK, &err));

    /* Zero request also defaults to the cap. */
    wf_ota_init(&o);
    build_start(start, 500, 0, sha);
    TEST_ASSERT_EQUAL_INT(WF_OTA_MAX_CHUNK,
        wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, WF_OTA_MAX_CHUNK, &err));

    /* A device/MTU cap smaller than the request wins. */
    wf_ota_init(&o);
    build_start(start, 500, 200, sha);
    TEST_ASSERT_EQUAL_INT(128,
        wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, 128, &err));
}

void test_bad_size(void)
{
    uint8_t sha[32] = {0};
    uint8_t start[WF_OTA_START_LEN]; uint8_t err = 0;
    wf_ota_t o; wf_ota_init(&o);

    build_start(start, 0, 244, sha);
    TEST_ASSERT_EQUAL_INT(-1, wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, 244, &err));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_BAD_SIZE, err);

    wf_ota_init(&o);
    build_start(start, SLOT_CAP + 1, 244, sha);
    TEST_ASSERT_EQUAL_INT(-1, wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, 244, &err));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_BAD_SIZE, err);
}

void test_out_of_order_rejected(void)
{
    uint8_t img[500];
    for (int i = 0; i < 500; i++) img[i] = (uint8_t)i;
    uint8_t sha[32]; sha_of(img, sizeof(img), sha);

    wf_ota_t o; wf_ota_init(&o);
    uint8_t start[WF_OTA_START_LEN]; uint8_t err = 0;
    build_start(start, sizeof(img), 100, sha);
    wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, 244, &err);

    /* Send seq 1 before seq 0 -> rejected. */
    uint8_t frame[2 + 100];
    frame[0] = 1; frame[1] = 0;
    memcpy(frame + 2, img, 100);
    TEST_ASSERT_EQUAL_INT(-1, wf_ota_feed(&o, frame, 2 + 100, &err));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_OUT_OF_ORDER, err);
    TEST_ASSERT_EQUAL_INT(WF_OTA_FAILED, o.state);
}

void test_overrun_rejected(void)
{
    uint8_t img[100];
    memset(img, 0xAB, sizeof(img));
    uint8_t sha[32]; sha_of(img, sizeof(img), sha);

    wf_ota_t o; wf_ota_init(&o);
    uint8_t start[WF_OTA_START_LEN]; uint8_t err = 0;
    build_start(start, sizeof(img), 244, sha);
    wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, 244, &err);

    /* One chunk claiming 150 bytes for a 100-byte image -> overrun. */
    uint8_t frame[2 + 150];
    frame[0] = 0; frame[1] = 0;
    memset(frame + 2, 0xAB, 150);
    TEST_ASSERT_EQUAL_INT(-1, wf_ota_feed(&o, frame, 2 + 150, &err));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_OUT_OF_ORDER, err);
}

void test_sha_mismatch(void)
{
    uint8_t img[300];
    for (int i = 0; i < 300; i++) img[i] = (uint8_t)(i ^ 0x5A);
    uint8_t sha[32]; sha_of(img, sizeof(img), sha);
    sha[0] ^= 0xFF;   /* corrupt the announced digest */

    wf_ota_t o; wf_ota_init(&o);
    TEST_ASSERT_EQUAL_INT(-1, run_transfer(img, sizeof(img), 128, sha, &o));
    TEST_ASSERT_EQUAL_INT(WF_OTA_FAILED, o.state);
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_SHA, o.last_error);
}

void test_commit_before_complete(void)
{
    uint8_t img[300];
    memset(img, 0x11, sizeof(img));
    uint8_t sha[32]; sha_of(img, sizeof(img), sha);

    wf_ota_t o; wf_ota_init(&o);
    uint8_t start[WF_OTA_START_LEN]; uint8_t err = 0;
    build_start(start, sizeof(img), 100, sha);
    wf_ota_begin(&o, start, WF_OTA_START_LEN, SLOT_CAP, 244, &err);

    uint8_t frame[2 + 100];
    frame[0] = 0; frame[1] = 0;
    memcpy(frame + 2, img, 100);
    wf_ota_feed(&o, frame, 2 + 100, &err);   /* only 100 of 300 */

    TEST_ASSERT_EQUAL_INT(-1, wf_ota_commit(&o, &err));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_OUT_OF_ORDER, err);
    TEST_ASSERT_FALSE(wf_ota_complete(&o));
}

void test_feed_in_wrong_state(void)
{
    wf_ota_t o; wf_ota_init(&o);   /* IDLE */
    uint8_t frame[4] = {0, 0, 0x55, 0x66};
    uint8_t err = 0;
    TEST_ASSERT_EQUAL_INT(-1, wf_ota_feed(&o, frame, 4, &err));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_STATE, err);
}

void test_frame_builders(void)
{
    uint8_t b[16];

    TEST_ASSERT_EQUAL_INT(WF_OTA_FRAME_READY_LEN, wf_ota_frame_ready(b, 0, 244, 8));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_NTF_READY, b[0]);
    TEST_ASSERT_EQUAL_UINT8(0, b[1]);
    TEST_ASSERT_EQUAL_UINT8(244, b[2]); TEST_ASSERT_EQUAL_UINT8(0, b[3]);
    TEST_ASSERT_EQUAL_UINT8(8, b[4]);   TEST_ASSERT_EQUAL_UINT8(0, b[5]);

    TEST_ASSERT_EQUAL_INT(WF_OTA_FRAME_CREDIT_LEN, wf_ota_frame_credit(b, 0x0102, 0x03040506, 4));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_NTF_CREDIT, b[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, b[1]); TEST_ASSERT_EQUAL_UINT8(0x01, b[2]);  /* seq LE */
    TEST_ASSERT_EQUAL_UINT8(0x06, b[3]); TEST_ASSERT_EQUAL_UINT8(0x05, b[4]);
    TEST_ASSERT_EQUAL_UINT8(0x04, b[5]); TEST_ASSERT_EQUAL_UINT8(0x03, b[6]);  /* recv LE */
    TEST_ASSERT_EQUAL_UINT8(0x04, b[7]); TEST_ASSERT_EQUAL_UINT8(0x00, b[8]);

    TEST_ASSERT_EQUAL_INT(WF_OTA_FRAME_DONE_LEN, wf_ota_frame_done(b, 0));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_NTF_DONE, b[0]);

    TEST_ASSERT_EQUAL_INT(WF_OTA_FRAME_ERROR_LEN, wf_ota_frame_error(b, WF_OTA_ERR_SHA));
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_NTF_ERROR, b[0]);
    TEST_ASSERT_EQUAL_UINT8(WF_OTA_ERR_SHA, b[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sha256_abc_vector);
    RUN_TEST(test_full_transfer_ok);
    RUN_TEST(test_chunk_clamp);
    RUN_TEST(test_bad_size);
    RUN_TEST(test_out_of_order_rejected);
    RUN_TEST(test_overrun_rejected);
    RUN_TEST(test_sha_mismatch);
    RUN_TEST(test_commit_before_complete);
    RUN_TEST(test_feed_in_wrong_state);
    RUN_TEST(test_frame_builders);
    return UNITY_END();
}
