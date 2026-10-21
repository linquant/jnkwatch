#pragma once
#include "lvgl.h"

/* Shared neon/tech palette used by every face. */
#define COL_BG      lv_color_hex(0x000000) /* pure black (AMOLED: unlit pixels) */
#define COL_CORE    lv_color_hex(0xEAFFFF) /* bright digit core       */
#define COL_GLOW    lv_color_hex(0x00E5FF) /* cyan halo               */
#define COL_SEC     lv_color_hex(0xFF2E9A) /* magenta accent          */
#define COL_TRACK   lv_color_hex(0x10202A) /* dim ring track          */
#define COL_LABEL   lv_color_hex(0x3D7E8C) /* dim tech labels         */
#define COL_OK      lv_color_hex(0x00E5FF) /* healthy / normal        */
#define COL_LOW     lv_color_hex(0xFFB020) /* low / warning           */
#define COL_CRIT    lv_color_hex(0xFF3B30) /* critical / done         */

/* Offsets for the stacked-copy text glow (soft halo). */
#define UI_GLOW_N 16
extern const int UI_GLOW_OFS[UI_GLOW_N][2];

/* --- Dual-display accent teintes (Casio-screen backlight colours) ---
 * The clock face keeps one layout and only swaps its accent hue: the hour
 * digit, the seconds, the first status dot and a faint backlight wash. */
typedef enum {
    ACCENT_ROSE = 0,
    ACCENT_VERT,
    ACCENT_CYAN,
    ACCENT_VIOLET,
    ACCENT_OR,
    ACCENT_BLEU,
    ACCENT_COUNT,
} accent_t;

lv_color_t   theme_accent_color(accent_t a); /* colour of a given teinte     */
const char  *theme_accent_name(accent_t a);  /* "Rose", "Vert", ...          */
void         theme_set_accent(accent_t a);   /* select current teinte (wraps)*/
accent_t     theme_get_accent(void);         /* current teinte index         */
lv_color_t   theme_accent(void);             /* colour of the current teinte */
