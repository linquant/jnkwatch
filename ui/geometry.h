#pragma once
/* Single source of layout geometry, derived from the panel size so faces are
 * authored against the real resolution instead of hardcoded pixel literals.
 * Flip WATCH_SIZE (platform/display.h) and every face follows. */
#include "display.h"

#define UI_W        WATCH_SIZE                  /* 466 */
#define UI_H        WATCH_SIZE
#define UI_CX       (WATCH_SIZE / 2)            /* 233 (center x) */
#define UI_CY       (WATCH_SIZE / 2)            /* 233 (center y) */
#define UI_R        (WATCH_SIZE / 2)            /* 233 (outer glass radius) */
#define UI_SAFE_R   ((WATCH_SIZE * 45) / 100)   /* ~210 content-safe radius (off the bezel) */

/* Max half-width of a horizontally-centered band at |y| = h from center,
 * so cards/rows can be sized to stay inside the safe circle. */
#define UI_CHORD_HALF(h) \
    ((int)__builtin_sqrt((double)UI_SAFE_R * UI_SAFE_R - (double)(h) * (h)))

/* Place big static buffers (e.g. a full-screen canvas) in external PSRAM on
 * device; a no-op on the PC build. Keeps internal DRAM from overflowing. */
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define WF_BIG_BSS EXT_RAM_BSS_ATTR
#else
#define WF_BIG_BSS
#endif
