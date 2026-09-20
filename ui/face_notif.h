#pragma once
#include "lvgl.h"

/* Notifications face: a round-optimised card showing the latest phone
 * notification (source + relative time, title, message preview, unread count).
 * Fed by the watch_poll_notification() HAL (BLE on hardware, a mock on PC). */
void face_notif_create(lv_obj_t *parent);
void face_notif_tick(void);
void face_notif_apply_accent(void);

/* Number of unread notifications (new arrivals not yet seen). Drives the
 * bell indicator on the clock face. */
int  face_notif_unread(void);

/* Mark all notifications as seen (called while the notifications face is on
 * screen), clearing the unread count. */
void face_notif_mark_read(void);
