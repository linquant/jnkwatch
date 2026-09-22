#include "display.h"
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Full-screen render buffer (LVGL renders the whole frame here in FULL mode). */
static uint16_t g_frame[WATCH_SIZE * WATCH_SIZE];

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    /* In LV_DISPLAY_RENDER_MODE_FULL the whole frame is rendered directly into
     * g_frame, so there is nothing to copy on the PC side. On real hardware
     * this is where we'd DMA px_map to the GC9A01 over SPI. */
    (void)area;
    (void)px_map;
    lv_display_flush_ready(disp);
}

lv_display_t *watch_display_init(void)
{
    lv_display_t *disp = lv_display_create(WATCH_SIZE, WATCH_SIZE);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, g_frame, NULL, sizeof(g_frame),
                           LV_DISPLAY_RENDER_MODE_FULL);
    return disp;
}

const uint16_t *watch_framebuffer(void)
{
    return g_frame;
}

uint32_t watch_millis(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

int watch_battery_pct(void)
{
    /* PC mock: a healthy, slowly-breathing level so the ring is visible.
     * On the ESP32-S3, read the battery divider via ADC here instead. */
    return 68;
}

double watch_heading_deg(void)
{
    /* PC mock: a slow, steady rotation so the compass visibly moves.
     * On hardware, replace with magnetometer (or gyro-integrated) heading. */
    return (double)((watch_millis() / 40) % 360);
}

void watch_set_backlight(int pct)
{
    /* PC: no real backlight (the sim shows brightness with a dim overlay).
     * On hardware: set LEDC PWM duty on the backlight GPIO from pct. */
    (void)pct;
}

void watch_set_wrist_wake(bool on)
{
    /* PC: nothing to do. On hardware: arm/disarm QMI8658 wake-on-motion. */
    (void)on;
}

void watch_read_accel(double *ax, double *ay, double *az)
{
    /* PC mock. Base posture: resting, with a brief "viewing" tilt every ~9s
     * (drives lift-to-wake). Plus an activity window in a 16s cycle that adds
     * shake (drives the living watchface). On hardware, read the QMI8658. */
    uint32_t t = watch_millis();
    double bx, by, bz;
    if (t % 9000 < 2500) { bx = 0.0; by = 0.55; bz = 0.83; } /* raised */
    else                 { bx = 0.0; by = 0.98; bz = 0.10; } /* resting */

    double ph = (double)(t % 16000);
    double env = (ph > 6000.0 && ph < 11000.0) ? 1.0 : 0.0;  /* active window */
    double j = env * 0.7 * sin(t * 0.05);

    *ax = bx + j;
    *ay = by;
    *az = bz + j * 0.6;
}

int watch_poll_notification(wf_notif_t *out)
{
    /* PC mock BLE feed: a scripted list delivered over time. The first few
     * appear quickly so the screen isn't empty, then one drips in periodically
     * so you can see a fresh notification arrive. On hardware, delete this and
     * push from the BLE notification callback instead. */
    static const struct { const char *app, *title, *body; } feed[] = {
        {"Messages", "Lea", "On se voit a 19h au cafe ? J'ai reserve une table."},
        {"Agenda", "Rappel", "Reunion equipe dans 15 min - salle B."},
        {"WhatsApp", "Groupe Rando", "Thomas: J'apporte le cafe et les gateaux !"},
        {"Mail", "Banque", "Votre releve du mois est disponible."},
        {"Telephone", "Appel manque", "Papa - il y a 2 minutes."},
    };
    const int nfeed = (int)(sizeof(feed) / sizeof(feed[0]));

    static int delivered = 0;
    static uint32_t next_at = 300;   /* backlog syncs ~0.3s after "connect" */
    if (delivered >= nfeed) return 0;

    uint32_t now = watch_millis();
    if (now < next_at) return 0;

    const int i = delivered++;
    snprintf(out->app, sizeof(out->app), "%s", feed[i].app);
    snprintf(out->title, sizeof(out->title), "%s", feed[i].title);
    snprintf(out->body, sizeof(out->body), "%s", feed[i].body);
    out->ts = (long)watch_time();
    out->id = 0; /* the store assigns the real id on push */

    /* First 3 arrive together (like a backlog sync on connect); the rest drip
     * in every ~9s so a fresh notification is visibly delivered. */
    next_at = (delivered < 3) ? now : now + 9000u;
    return 1;
}

void watch_send_action(unsigned action_id)
{
    /* PC: nothing to send (the sim flashes the pressed button as feedback).
     * On hardware: write action_id to the phone over BLE (GATT). */
    (void)action_id;
}

time_t watch_time(void)
{
    /* PC: host clock, overridable via $WATCH_EPOCH for deterministic shots.
     * On hardware: read the PCF85063 RTC (I2C 0x51) and convert to epoch. */
    const char *e = getenv("WATCH_EPOCH");
    if (e && *e) return (time_t)strtoll(e, NULL, 10);
    return time(NULL);
}

void watch_now(struct tm *out)
{
    time_t t = watch_time();
    localtime_r(&t, out);
}
