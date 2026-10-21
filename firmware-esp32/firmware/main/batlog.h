#pragma once
/* Battery log: one 32-byte record per minute (+ one per power event) in a
 * circular flash partition ("batlog", 2 MB ≈ 65536 records ≈ 45 days).
 * Read back over USB with tools/batlog.sh (esptool, no app needed). */
#include <stdint.h>
#include <stdbool.h>

#define BATLOG_F_EXT     0x01  /* external power present            */
#define BATLOG_F_BLE     0x02  /* phone connected                   */
#define BATLOG_F_BOOT    0x04  /* event: first record after boot    */
#define BATLOG_F_PLUG    0x08  /* event: charger plugged            */
#define BATLOG_F_UNPLUG  0x10  /* event: charger unplugged          */
#define BATLOG_F_USB     0x20  /* USB host connected (no light-sleep then) */

#define BATLOG_VERSION   2

/* On-flash layout — tools/batlog_decode.py mirrors it, keep them in sync. */
typedef struct __attribute__((packed)) {
    uint32_t time;      /* UTC epoch (RTC)                              */
    uint32_t seq;       /* monotonic record number                      */
    uint16_t mv;        /* raw battery reading in mV                    */
    int8_t   pct;       /* charge shown, -1 = unknown                   */
    uint8_t  flags;     /* BATLOG_F_*                                   */
    uint8_t  screen_s;  /* seconds the screen was on since last record  */
    uint8_t  bright;    /* brightness % at record time                  */
    uint8_t  period_s;  /* seconds covered by this record (<= 60)       */
    uint8_t  ver;       /* BATLOG_VERSION                               */
    uint16_t sleep_pm;  /* time spent in auto light-sleep, per mille    */
    uint16_t sleeps;    /* light-sleep entries during the period        */
    uint8_t  rsv[11];   /* 0xFF, room for future fields                 */
    uint8_t  crc;       /* CRC-8 of the 31 bytes above                  */
} batlog_rec_t;

_Static_assert(sizeof(batlog_rec_t) == 32, "batlog record must be 32 bytes");

/* Locate the partition and the write head. False if the partition is missing
 * (old partition table) — logging is then silently disabled. */
bool batlog_init(void);

/* Append a record; fills seq, ver + crc. Call from a single task. */
void batlog_append(batlog_rec_t *r);
