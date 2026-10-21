#pragma once
/* Pure probe store: the latest values pushed from the phone for the "Sondes"
 * watchface. Each enabled GET probe on the phone (ProbeRunner) owns a slot
 * (0..WF_PROBE_MAX-1); the phone writes name+value over BLE and the watch
 * timestamps the reception with its own clock, so the face can show how fresh
 * each value is. NO LVGL/platform dependency, unit-tested (tests/test_probes.c).
 * The BLE layer writes it via wf_probes_set; the face only reads it. The watch
 * never fetches anything itself — the phone is the source. */
#include <stdbool.h>
#include <stddef.h>

#define WF_PROBE_MAX        10   /* must match GetProbe.MAX on the phone     */
#define WF_PROBE_NAME_LEN   24   /* slot title, incl. NUL                    */
#define WF_PROBE_VALUE_LEN  48   /* extracted value, incl. NUL              */

typedef struct {
    char          name[WF_PROBE_NAME_LEN];
    char          value[WF_PROBE_VALUE_LEN];
    unsigned long updated_ms;   /* watch clock at last set (0 when unused)  */
    bool          used;
} wf_probe_slot_t;

typedef struct {
    wf_probe_slot_t slot[WF_PROBE_MAX];
} wf_probes_t;

/* Reset every slot to "no data". */
void wf_probes_init(wf_probes_t *p);

/* Store/refresh slot [i] with name+value, stamped at now_ms. An out-of-range
 * slot or an empty/NULL name is ignored; a NULL value is stored as "". */
void wf_probes_set(wf_probes_t *p, int slot, const char *name,
                   const char *value, unsigned long now_ms);

/* Mark slot [i] empty (the phone stopped sending it). */
void wf_probes_clear(wf_probes_t *p, int slot);

/* Number of used slots. */
int wf_probes_count(const wf_probes_t *p);

/* Write a short ASCII French "freshness" string for a value last updated at
 * updated_ms, given the current now_ms: "a l'instant", "il y a 5 s",
 * "il y a 5 min", "il y a 2 h", "il y a 3 j". Always NUL-terminates. */
void wf_probes_format_age(char *buf, size_t cap,
                          unsigned long updated_ms, unsigned long now_ms);
