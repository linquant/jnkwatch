/* Minimal LVGL v9 configuration for the watch project.
 * Only overrides what we care about; lv_conf_internal.h fills in the rest. */
#ifndef LV_CONF_H
#define LV_CONF_H

/* Match the GC9A01 panel: RGB565 (16-bit) */
#define LV_COLOR_DEPTH 16

/* Software renderer (no GPU on the PC sim nor on the ESP32-S3) */
#define LV_USE_DRAW_SW 1

/* Use the system allocator so heavy canvas drawing isn't capped by the small
 * built-in heap (the digit canvas issues many draw tasks). */
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING  LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* Keep logging quiet by default (set to 1 + LV_LOG_LEVEL to debug) */
#define LV_USE_LOG 0

/* Fonts we use on the watchface (466px screen: bigger sizes) */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_28

/* Widgets we rely on */
#define LV_USE_LINE   1
#define LV_USE_ARC    1
#define LV_USE_LABEL  1

#endif /* LV_CONF_H */
