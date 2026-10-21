#pragma once
/* Pure OTA (over-the-air firmware update) wire protocol + transfer state machine.
 *
 * This module owns the *logic* of a BLE OTA transfer with NO platform/ESP/flash
 * dependency, so it is unit-tested on the PC build (tests/test_ota_proto.c). The
 * device layer (firmware/main/ota_update.c) drives the real flash via esp_ota_*
 * and uses this module to: validate the START request, check each DATA chunk is
 * in order and within bounds, hash the received image (SHA-256) to compare with
 * the digest the phone announced, and build the notification frames sent back.
 *
 * Wire format mirrors docs/ble-protocol.md (protocol v3), chars OTA_CTRL (0x0A)
 * and OTA_DATA (0x0B). All multi-byte integers are little-endian.
 *
 *   OTA_CTRL  phone -> watch (writes):
 *     START : [0x01][total_size u32][chunk_size u16][sha256 32B]   (39 bytes)
 *     ABORT : [0x02]
 *     COMMIT: [0x03]
 *   OTA_CTRL  watch -> phone (notifications):
 *     READY : [0x81][status u8][chunk_size u16][window u16]        (6 bytes)
 *     CREDIT: [0x82][next_seq u16][bytes_recvd u32][credits u16]   (9 bytes)
 *     DONE  : [0x83][status u8]                                    (2 bytes)
 *     ERROR : [0x84][code u8]                                      (2 bytes)
 *   OTA_DATA  phone -> watch (write-no-response):
 *     [seq u16][payload ...]   seq is the 0-based chunk index.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sha256.h"

/* Control opcodes (first byte of an OTA_CTRL write). */
#define WF_OTA_OP_START   0x01
#define WF_OTA_OP_ABORT   0x02
#define WF_OTA_OP_COMMIT  0x03

/* Notification codes (first byte of an OTA_CTRL notify). */
#define WF_OTA_NTF_READY  0x81
#define WF_OTA_NTF_CREDIT 0x82
#define WF_OTA_NTF_DONE   0x83
#define WF_OTA_NTF_ERROR  0x84

/* Error codes carried by ERROR / returned via out_err. 0 = no error. */
#define WF_OTA_ERR_NONE         0
#define WF_OTA_ERR_BAD_SIZE     1  /* total_size is 0 or exceeds the slot        */
#define WF_OTA_ERR_FLASH        2  /* device-side esp_ota_write/begin failed      */
#define WF_OTA_ERR_SHA          3  /* final digest mismatch                       */
#define WF_OTA_ERR_LOW_BATTERY  4  /* refused: not charging and battery too low   */
#define WF_OTA_ERR_OUT_OF_ORDER 5  /* unexpected chunk seq, or overrun            */
#define WF_OTA_ERR_TIMEOUT      6  /* no frame within the device timeout          */
#define WF_OTA_ERR_BEGIN        7  /* device-side esp_ota_begin failed            */
#define WF_OTA_ERR_STATE        8  /* op not valid in the current state           */

#define WF_OTA_SHA_LEN      WF_SHA256_LEN   /* 32 */
#define WF_OTA_START_LEN    39              /* 1 + 4 + 2 + 32 */
#define WF_OTA_DATA_HDR     2               /* seq u16 */
#define WF_OTA_MAX_CHUNK    244             /* fits ATT payload at MTU 247        */
#define WF_OTA_DEFAULT_WINDOW 8             /* in-flight chunk credits            */

/* Frame buffer sizes for the notify builders. */
#define WF_OTA_FRAME_READY_LEN  6
#define WF_OTA_FRAME_CREDIT_LEN 9
#define WF_OTA_FRAME_DONE_LEN   2
#define WF_OTA_FRAME_ERROR_LEN  2

typedef enum {
    WF_OTA_IDLE = 0,  /* no transfer in progress            */
    WF_OTA_ACTIVE,    /* START accepted, receiving chunks    */
    WF_OTA_DONE,      /* COMMIT succeeded, digest verified   */
    WF_OTA_FAILED,    /* aborted or errored                  */
} wf_ota_state_t;

typedef struct {
    wf_ota_state_t state;
    uint32_t    total_size;    /* announced image size in bytes     */
    uint32_t    received;      /* bytes accepted so far             */
    uint16_t    chunk_size;    /* negotiated payload size per chunk */
    uint16_t    next_seq;      /* expected seq of the next chunk    */
    uint8_t     expected_sha[WF_OTA_SHA_LEN];
    wf_sha256_t sha;           /* streaming hash of received bytes  */
    uint8_t     last_error;    /* WF_OTA_ERR_* of the last failure  */
} wf_ota_t;

/* Reset to IDLE. */
void wf_ota_init(wf_ota_t *o);

/* Parse+validate a START frame (len must be WF_OTA_START_LEN). slot_capacity is
 * the destination partition size; max_chunk caps the negotiated chunk (pass
 * WF_OTA_MAX_CHUNK or a device/MTU-derived value). On success: state -> ACTIVE,
 * returns the negotiated chunk_size (>0). On failure: returns -1 and sets
 * *out_err (BAD_SIZE / STATE). Does not itself enforce battery; the caller may
 * reject with WF_OTA_ERR_LOW_BATTERY before calling this. */
int wf_ota_begin(wf_ota_t *o, const uint8_t *frame, int len,
                 uint32_t slot_capacity, uint16_t max_chunk, uint8_t *out_err);

/* Feed one OTA_DATA frame ([seq u16][payload]). Validates state==ACTIVE, that
 * seq == next_seq, and that received+payload <= total_size, then updates the
 * running SHA and counters. Returns the payload byte count on success, or -1 and
 * sets *out_err (STATE / OUT_OF_ORDER). The caller flashes the payload. */
int wf_ota_feed(wf_ota_t *o, const uint8_t *frame, int len, uint8_t *out_err);

/* True once all announced bytes have been received (ready to COMMIT). */
bool wf_ota_complete(const wf_ota_t *o);

/* Finalize: require received==total_size, compute the digest and compare it to
 * the announced one. On match: state -> DONE, returns 0. On mismatch/short:
 * state -> FAILED, returns -1 and sets *out_err (SHA / OUT_OF_ORDER / STATE). */
int wf_ota_commit(wf_ota_t *o, uint8_t *out_err);

/* Mark the transfer failed with a code (ABORT, timeout, flash error, ...). */
void wf_ota_fail(wf_ota_t *o, uint8_t err);

/* Notification frame builders. Each writes a fixed number of bytes (the
 * WF_OTA_FRAME_*_LEN above) into buf and returns that length. */
int wf_ota_frame_ready(uint8_t *buf, uint8_t status, uint16_t chunk_size, uint16_t window);
int wf_ota_frame_credit(uint8_t *buf, uint16_t next_seq, uint32_t received, uint16_t credits);
int wf_ota_frame_done(uint8_t *buf, uint8_t status);
int wf_ota_frame_error(uint8_t *buf, uint8_t code);
