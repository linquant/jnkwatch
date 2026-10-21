#include "theme.h"

/* Stacked-copy glow offsets, shared by the faces that halo their text. */
const int UI_GLOW_OFS[UI_GLOW_N][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1},
    {2, 0}, {-2, 0}, {0, 2}, {0, -2}, {2, 2}, {-2, -2}, {2, -2}, {-2, 2},
};

/* Six teintes, in enum order, tuned to read like Casio LCD backlights. */
static const uint32_t ACCENTS[ACCENT_COUNT] = {
    [ACCENT_ROSE]   = 0xFF3D9A, /* pink        */
    [ACCENT_VERT]   = 0x54E08A, /* green       */
    [ACCENT_CYAN]   = 0x37DCE0, /* cyan        */
    [ACCENT_VIOLET] = 0xB57BFF, /* violet      */
    [ACCENT_OR]     = 0xE6C34C, /* gold        */
    [ACCENT_BLEU]   = 0x5B8CF5, /* blue        */
};

static const char *ACCENT_NAMES[ACCENT_COUNT] = {
    [ACCENT_ROSE]   = "Rose",
    [ACCENT_VERT]   = "Vert",
    [ACCENT_CYAN]   = "Cyan",
    [ACCENT_VIOLET] = "Violet",
    [ACCENT_OR]     = "Or",
    [ACCENT_BLEU]   = "Bleu",
};

static accent_t g_accent = ACCENT_CYAN;

lv_color_t theme_accent_color(accent_t a)
{
    if (a < 0 || a >= ACCENT_COUNT) a = ACCENT_CYAN;
    return lv_color_hex(ACCENTS[a]);
}

const char *theme_accent_name(accent_t a)
{
    if (a < 0 || a >= ACCENT_COUNT) a = ACCENT_CYAN;
    return ACCENT_NAMES[a];
}

void theme_set_accent(accent_t a)
{
    g_accent = (accent_t)(((int)a % ACCENT_COUNT + ACCENT_COUNT) % ACCENT_COUNT);
}

accent_t theme_get_accent(void)
{
    return g_accent;
}

lv_color_t theme_accent(void)
{
    return theme_accent_color(g_accent);
}
