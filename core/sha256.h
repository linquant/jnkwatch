#pragma once
/* Minimal streaming SHA-256 (FIPS 180-4), public-domain implementation adapted
 * from Brad Conte's crypto-algorithms (github.com/B-Con/crypto-algorithms).
 * Vendored so core/ota_proto can hash the incoming firmware identically on the
 * PC unit-test build and on the device, with no platform/mbedTLS dependency. */
#include <stddef.h>
#include <stdint.h>

#define WF_SHA256_LEN 32   /* digest length in bytes */

typedef struct {
    uint8_t  data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} wf_sha256_t;

void wf_sha256_init(wf_sha256_t *c);
void wf_sha256_update(wf_sha256_t *c, const uint8_t *data, size_t len);
/* Writes WF_SHA256_LEN bytes into out. The context must not be reused after. */
void wf_sha256_final(wf_sha256_t *c, uint8_t out[WF_SHA256_LEN]);
