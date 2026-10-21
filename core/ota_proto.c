#include "ota_proto.h"
#include <string.h>

/* --- little-endian helpers ------------------------------------------------- */
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void wr_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void wr_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;         p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

void wf_ota_init(wf_ota_t *o)
{
    memset(o, 0, sizeof(*o));
    o->state = WF_OTA_IDLE;
}

void wf_ota_fail(wf_ota_t *o, uint8_t err)
{
    o->state = WF_OTA_FAILED;
    o->last_error = err;
}

int wf_ota_begin(wf_ota_t *o, const uint8_t *frame, int len,
                 uint32_t slot_capacity, uint16_t max_chunk, uint8_t *out_err)
{
    if (len != WF_OTA_START_LEN || frame[0] != WF_OTA_OP_START) {
        if (out_err) *out_err = WF_OTA_ERR_STATE;
        return -1;
    }
    uint32_t total = rd_u32(frame + 1);
    uint16_t req_chunk = rd_u16(frame + 5);

    if (total == 0 || total > slot_capacity) {
        wf_ota_fail(o, WF_OTA_ERR_BAD_SIZE);
        if (out_err) *out_err = WF_OTA_ERR_BAD_SIZE;
        return -1;
    }

    uint16_t cap = max_chunk ? max_chunk : WF_OTA_MAX_CHUNK;
    if (cap > WF_OTA_MAX_CHUNK) cap = WF_OTA_MAX_CHUNK;
    uint16_t chunk = req_chunk;
    if (chunk == 0 || chunk > cap) chunk = cap;

    o->state = WF_OTA_ACTIVE;
    o->total_size = total;
    o->received = 0;
    o->chunk_size = chunk;
    o->next_seq = 0;
    o->last_error = WF_OTA_ERR_NONE;
    memcpy(o->expected_sha, frame + 7, WF_OTA_SHA_LEN);
    wf_sha256_init(&o->sha);

    if (out_err) *out_err = WF_OTA_ERR_NONE;
    return (int)chunk;
}

int wf_ota_feed(wf_ota_t *o, const uint8_t *frame, int len, uint8_t *out_err)
{
    if (o->state != WF_OTA_ACTIVE) {
        if (out_err) *out_err = WF_OTA_ERR_STATE;
        return -1;
    }
    if (len < WF_OTA_DATA_HDR) {
        wf_ota_fail(o, WF_OTA_ERR_OUT_OF_ORDER);
        if (out_err) *out_err = WF_OTA_ERR_OUT_OF_ORDER;
        return -1;
    }
    uint16_t seq = rd_u16(frame);
    int payload = len - WF_OTA_DATA_HDR;

    if (seq != o->next_seq ||
        (uint32_t)payload > o->total_size - o->received) {
        wf_ota_fail(o, WF_OTA_ERR_OUT_OF_ORDER);
        if (out_err) *out_err = WF_OTA_ERR_OUT_OF_ORDER;
        return -1;
    }

    if (payload > 0)
        wf_sha256_update(&o->sha, frame + WF_OTA_DATA_HDR, (size_t)payload);
    o->received += (uint32_t)payload;
    o->next_seq++;

    if (out_err) *out_err = WF_OTA_ERR_NONE;
    return payload;
}

bool wf_ota_complete(const wf_ota_t *o)
{
    return o->state == WF_OTA_ACTIVE && o->received == o->total_size;
}

int wf_ota_commit(wf_ota_t *o, uint8_t *out_err)
{
    if (o->state != WF_OTA_ACTIVE) {
        if (out_err) *out_err = WF_OTA_ERR_STATE;
        return -1;
    }
    if (o->received != o->total_size) {
        wf_ota_fail(o, WF_OTA_ERR_OUT_OF_ORDER);
        if (out_err) *out_err = WF_OTA_ERR_OUT_OF_ORDER;
        return -1;
    }
    uint8_t digest[WF_OTA_SHA_LEN];
    wf_sha256_final(&o->sha, digest);
    if (memcmp(digest, o->expected_sha, WF_OTA_SHA_LEN) != 0) {
        wf_ota_fail(o, WF_OTA_ERR_SHA);
        if (out_err) *out_err = WF_OTA_ERR_SHA;
        return -1;
    }
    o->state = WF_OTA_DONE;
    o->last_error = WF_OTA_ERR_NONE;
    if (out_err) *out_err = WF_OTA_ERR_NONE;
    return 0;
}

int wf_ota_frame_ready(uint8_t *buf, uint8_t status, uint16_t chunk_size, uint16_t window)
{
    buf[0] = WF_OTA_NTF_READY;
    buf[1] = status;
    wr_u16(buf + 2, chunk_size);
    wr_u16(buf + 4, window);
    return WF_OTA_FRAME_READY_LEN;
}

int wf_ota_frame_credit(uint8_t *buf, uint16_t next_seq, uint32_t received, uint16_t credits)
{
    buf[0] = WF_OTA_NTF_CREDIT;
    wr_u16(buf + 1, next_seq);
    wr_u32(buf + 3, received);
    wr_u16(buf + 7, credits);
    return WF_OTA_FRAME_CREDIT_LEN;
}

int wf_ota_frame_done(uint8_t *buf, uint8_t status)
{
    buf[0] = WF_OTA_NTF_DONE;
    buf[1] = status;
    return WF_OTA_FRAME_DONE_LEN;
}

int wf_ota_frame_error(uint8_t *buf, uint8_t code)
{
    buf[0] = WF_OTA_NTF_ERROR;
    buf[1] = code;
    return WF_OTA_FRAME_ERROR_LEN;
}
