/* Circular battery log in a raw flash partition — see batlog.h.
 *
 * NOR flash can program erased (0xFF) bytes without an erase, so each record
 * is written in place as it happens (nothing lost on reset) and a 4 KB sector
 * is erased only when the head enters it: once per ~2 h, i.e. each sector is
 * erased about every 45 days — negligible wear. Records of another version
 * (older layout) fail validation and are simply overwritten. The head is found at boot from
 * the highest valid seq. */
#include "batlog.h"
#include "esp_partition.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "batlog";

#define SECTOR      4096
#define PER_SECTOR  (SECTOR / sizeof(batlog_rec_t))

static const esp_partition_t *s_part;
static uint32_t s_nsec;       /* sectors in the partition        */
static uint32_t s_head;       /* next record slot (index)        */
static uint32_t s_seq;        /* seq of the next record          */

static uint8_t crc8(const uint8_t *p, int n)
{
    uint8_t c = 0;
    while (n--) {
        c ^= *p++;
        for (int i = 0; i < 8; i++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
    }
    return c;
}

static bool read_rec(uint32_t idx, batlog_rec_t *r)
{
    if (esp_partition_read(s_part, idx * sizeof(*r), r, sizeof(*r)) != ESP_OK) return false;
    return crc8((const uint8_t *)r, sizeof(*r) - 1) == r->crc &&
           r->ver == BATLOG_VERSION && r->seq != 0xFFFFFFFFu;
}

static bool is_blank(uint32_t idx)
{
    uint8_t b[sizeof(batlog_rec_t)];
    if (esp_partition_read(s_part, idx * sizeof(b), b, sizeof(b)) != ESP_OK) return false;
    for (size_t i = 0; i < sizeof(b); i++) if (b[i] != 0xFF) return false;
    return true;
}

bool batlog_init(void)
{
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "batlog");
    if (!s_part) {
        ESP_LOGW(TAG, "no 'batlog' partition — battery log disabled");
        return false;
    }
    s_nsec = s_part->size / SECTOR;

    /* The newest sector is the one whose first record has the highest seq. */
    int best = -1;
    uint32_t best_seq = 0;
    batlog_rec_t r;
    for (uint32_t s = 0; s < s_nsec; s++) {
        if (read_rec(s * PER_SECTOR, &r) && (best < 0 || r.seq > best_seq)) {
            best = (int)s;
            best_seq = r.seq;
        }
    }
    if (best < 0) {
        s_head = 0;
        s_seq = 0;
    } else {
        /* Head = just past the last written slot of that sector (a torn,
         * half-written record from a power cut still counts as written). */
        uint32_t first = (uint32_t)best * PER_SECTOR, last = first;
        s_seq = best_seq + 1;
        for (uint32_t idx = first; idx < first + PER_SECTOR; idx++) {
            if (read_rec(idx, &r) && r.seq >= s_seq) s_seq = r.seq + 1;
            if (!is_blank(idx)) last = idx;
        }
        s_head = (last + 1) % (s_nsec * PER_SECTOR);
    }
    ESP_LOGI(TAG, "partition %u KB @0x%lx, head %lu, next seq %lu",
             (unsigned)(s_part->size / 1024), (unsigned long)s_part->address,
             (unsigned long)s_head, (unsigned long)s_seq);
    return true;
}

void batlog_append(batlog_rec_t *r)
{
    if (!s_part) return;
    const uint32_t total = s_nsec * PER_SECTOR;
    /* Skip slots that aren't blank mid-sector (never erase a live sector). */
    while (s_head % PER_SECTOR != 0 && !is_blank(s_head)) s_head = (s_head + 1) % total;
    /* Entering a sector: erase it — this drops the oldest ~4 h of the ring. */
    if (s_head % PER_SECTOR == 0 &&
        esp_partition_erase_range(s_part, (s_head / PER_SECTOR) * SECTOR, SECTOR) != ESP_OK)
        return;
    r->seq = s_seq;
    r->ver = BATLOG_VERSION;
    r->crc = crc8((const uint8_t *)r, sizeof(*r) - 1);
    if (esp_partition_write(s_part, s_head * sizeof(*r), r, sizeof(*r)) != ESP_OK) {
        ESP_LOGW(TAG, "write failed at %lu", (unsigned long)s_head);
        return;
    }
    s_seq++;
    s_head = (s_head + 1) % total;
}
