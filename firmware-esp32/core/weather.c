#include "weather.h"
#include <stddef.h>
#include <stdio.h>

static void copy_field(char *dst, size_t cap, const char *src)
{
    if (!src) { dst[0] = '\0'; return; }
    /* snprintf always NUL-terminates and truncates to cap-1 chars. */
    snprintf(dst, cap, "%s", src);
}

void wf_weather_init(wf_weather_t *w)
{
    w->temp_c = 0;
    w->cond = WF_WX_CLOUDY;
    w->location[0] = '\0';
    w->valid = false;
}

void wf_weather_set(wf_weather_t *w, int temp_c, int cond, const char *location)
{
    w->temp_c = temp_c;
    w->cond = (cond < 0 || cond >= WF_WX_COND_COUNT) ? WF_WX_CLOUDY : cond;
    copy_field(w->location, sizeof(w->location), location);
    w->valid = true;
}
