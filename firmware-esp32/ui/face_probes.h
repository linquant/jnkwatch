#pragma once
#include "lvgl.h"

/* "Sondes" app: a scrolling list of the server probe values pushed from the
 * phone (fr.ponera.jnkwatch ProbeRunner). Each row shows the probe name, its
 * latest value and how long ago it was refreshed — the value updates at the
 * phone's per-probe polling frequency, with no notification spawned. */
void face_probes_create(lv_obj_t *parent);

/* Store/refresh one slot (0..WF_PROBE_MAX-1) with a name+value pushed over BLE,
 * stamped with the watch clock. */
void face_probes_set(int slot, const char *name, const char *value);

/* Refresh the "updated N min ago" labels. Call periodically while visible. */
void face_probes_tick(void);

/* Re-tint the accent-coloured probe names. */
void face_probes_apply_accent(void);
