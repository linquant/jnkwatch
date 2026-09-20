#include "actions.h"
#include <stdio.h>

static void copy_field(char *dst, size_t cap, const char *src)
{
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, cap, "%s", src); /* always NUL-terminates, truncates to cap-1 */
}

static wf_icon_t clamp_icon(wf_icon_t icon)
{
    return (icon < 0 || icon >= WF_ICON_COUNT) ? WF_ICON_NONE : icon;
}

void wf_actions_clear(wf_actions_t *s)
{
    s->count = 0;
}

int wf_actions_add(wf_actions_t *s, unsigned id, const char *label,
                   wf_icon_t icon, bool enabled)
{
    if (s->count >= WF_ACTION_MAX) return -1;
    int i = s->count++;
    wf_action_t *a = &s->items[i];
    a->id = id;
    copy_field(a->label, sizeof(a->label), label);
    a->icon = clamp_icon(icon);
    a->enabled = enabled;
    return i;
}

int wf_actions_set(wf_actions_t *s, int i, unsigned id, const char *label,
                   wf_icon_t icon, bool enabled)
{
    if (i < 0 || i >= s->count) return -1;
    wf_action_t *a = &s->items[i];
    a->id = id;
    copy_field(a->label, sizeof(a->label), label);
    a->icon = clamp_icon(icon);
    a->enabled = enabled;
    return i;
}

void wf_actions_set_enabled(wf_actions_t *s, int i, bool on)
{
    if (i < 0 || i >= s->count) return;
    s->items[i].enabled = on;
}

void wf_actions_init(wf_actions_t *s)
{
    wf_actions_clear(s);
    /* Default set (6 → one 2x3 page): generic placeholders until the phone
     * customizes them over BLE (via clear + add/set). */
    for (int i = 0; i < WF_ACTIONS_PER_PAGE; i++) {
        char label[WF_ACTION_LABEL_LEN];
        snprintf(label, sizeof(label), "btn%d", i + 1);
        wf_actions_add(s, (unsigned)(i + 1), label, WF_ICON_GENERIC, true);
    }
}

int wf_actions_count(const wf_actions_t *s)
{
    return s->count;
}

const wf_action_t *wf_actions_get(const wf_actions_t *s, int i)
{
    if (i < 0 || i >= s->count) return NULL;
    return &s->items[i];
}

int wf_actions_page_count(const wf_actions_t *s)
{
    if (s->count <= 0) return 1;
    return (s->count + WF_ACTIONS_PER_PAGE - 1) / WF_ACTIONS_PER_PAGE;
}
