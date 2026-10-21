#include "notify.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* --- UTF-8 helpers for notification sanitising (issue #20) ----------------- */

/* Decode one UTF-8 codepoint at p (NUL-terminated). Writes the codepoint to
 * *cp and returns the number of bytes consumed (1..4). On a malformed or
 * truncated sequence it consumes a single byte and reports *cp = 0 (dropped). */
static int utf8_decode(const unsigned char *p, uint32_t *cp)
{
    unsigned char c = p[0];
    if (c < 0x80) { *cp = c; return 1; }
    int n; uint32_t v;
    if      ((c & 0xE0) == 0xC0) { n = 1; v = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { n = 2; v = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { n = 3; v = c & 0x07; }
    else { *cp = 0; return 1; }                 /* stray continuation / invalid */
    for (int i = 1; i <= n; i++) {
        if ((p[i] & 0xC0) != 0x80) { *cp = 0; return 1; }  /* truncated */
        v = (v << 6) | (p[i] & 0x3F);
    }
    *cp = v;
    return n + 1;
}

/* Transliterate an accented Latin codepoint to its ASCII base (the Montserrat
 * font the watch renders text with has NO accented glyphs — the whole UI is
 * written accent-free for the same reason). Returns a short ASCII string, or
 * NULL for codepoints that have no sensible ASCII form (they are dropped). */
static const char *translit_latin(uint32_t cp)
{
    switch (cp) {
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: return "A";
    case 0xC6: return "AE";
    case 0xC7: return "C";
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: return "E";
    case 0xCC: case 0xCD: case 0xCE: case 0xCF: return "I";
    case 0xD0: return "D";
    case 0xD1: return "N";
    case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD8: return "O";
    case 0xD9: case 0xDA: case 0xDB: case 0xDC: return "U";
    case 0xDD: return "Y";
    case 0xDF: return "ss";
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: return "a";
    case 0xE6: return "ae";
    case 0xE7: return "c";
    case 0xE8: case 0xE9: case 0xEA: case 0xEB: return "e";
    case 0xEC: case 0xED: case 0xEE: case 0xEF: return "i";
    case 0xF0: return "d";
    case 0xF1: return "n";
    case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF8: return "o";
    case 0xF9: case 0xFA: case 0xFB: case 0xFC: return "u";
    case 0xFD: case 0xFF: return "y";
    case 0x152: return "OE";
    case 0x153: return "oe";
    case 0x160: return "S";  case 0x161: return "s";
    case 0x178: return "Y";
    case 0x17D: return "Z";  case 0x17E: return "z";
    case 0x20AC: return "EUR";   /* euro sign */
    default: return NULL;
    }
}

/* Copy src into dst[cap] as clean, renderable text. The Montserrat font only
 * has ASCII glyphs, so the output is pure ASCII: accented Latin is transliterated
 * (é->e, à->a, ç->c, œ->oe, €->EUR, ...), typographic punctuation and guillemets
 * are folded (' ' -> ', " " « » -> ", – — -> -, … -> ...), whitespace is
 * collapsed to single spaces, and emoji / pictographs / other symbols are
 * dropped. Truncation lands on a codepoint boundary, never mid-character. */
static void copy_field(char *dst, size_t cap, const char *src)
{
    if (cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }

    size_t w = 0;
    int prev_space = 1;   /* start "spaced" so leading whitespace is trimmed */
    const unsigned char *p = (const unsigned char *)src;

    while (*p) {
        uint32_t cp;
        p += utf8_decode(p, &cp);

        char out[4];
        int olen = 0;
        int is_space = 0;

        if (cp == 0) continue;                      /* invalid byte: drop */
        else if (cp == ' ' || cp == '\t' || cp == '\n' ||
                 cp == '\r' || cp == 0x00A0) is_space = 1;   /* + no-break space */
        else if (cp < 0x20) continue;               /* other control: drop */
        else if (cp < 0x7F)              { out[0] = (char)cp; olen = 1; }
        else if (cp == 0x2018 || cp == 0x2019) { out[0] = '\''; olen = 1; }   /* ' ' */
        else if (cp == 0x201C || cp == 0x201D ||
                 cp == 0x00AB || cp == 0x00BB)  { out[0] = '"';  olen = 1; }   /* " " « » */
        else if (cp == 0x2013 || cp == 0x2014) { out[0] = '-';  olen = 1; }   /* – — */
        else if (cp == 0x2026)           { out[0] = out[1] = out[2] = '.'; olen = 3; } /* … */
        else {                                      /* accented Latin -> ASCII, else drop */
            const char *a = translit_latin(cp);
            if (!a) continue;                       /* emoji / symbols / CJK: drop */
            while (a[olen]) { out[olen] = a[olen]; olen++; }
        }

        if (is_space) {
            if (prev_space) continue;               /* collapse runs of whitespace */
            out[0] = ' '; olen = 1; prev_space = 1;
        } else {
            prev_space = 0;
        }

        if (w + (size_t)olen >= cap) break;         /* boundary-safe truncation */
        for (int i = 0; i < olen; i++) dst[w++] = out[i];
    }

    while (w > 0 && dst[w - 1] == ' ') w--;          /* trim trailing space */
    dst[w] = '\0';
}

void wf_notif_init(wf_notif_store_t *s)
{
    s->count = 0;
    s->head = 0;
    s->next_id = 1;
}

unsigned wf_notif_push(wf_notif_store_t *s, const char *app,
                       const char *title, const char *body, long ts)
{
    /* head walks backwards so items[head] is always the newest; the physical
     * slot of the (now overwritten) oldest is reused when the buffer is full. */
    s->head = (s->head - 1 + WF_NOTIF_MAX) % WF_NOTIF_MAX;
    wf_notif_t *n = &s->items[s->head];

    copy_field(n->app, sizeof(n->app), app);
    copy_field(n->title, sizeof(n->title), title);
    copy_field(n->body, sizeof(n->body), body);
    n->ts = ts;
    n->id = s->next_id++;

    if (s->count < WF_NOTIF_MAX) s->count++;
    return n->id;
}

int wf_notif_count(const wf_notif_store_t *s)
{
    return s->count;
}

const wf_notif_t *wf_notif_get(const wf_notif_store_t *s, int i)
{
    if (i < 0 || i >= s->count) return NULL;
    int idx = (s->head + i) % WF_NOTIF_MAX;
    return &s->items[idx];
}

int wf_notif_remove(wf_notif_store_t *s, int i)
{
    if (i < 0 || i >= s->count) return s->count;
    /* Shift the newer-to-older run down by one over the removed slot; the
     * newest stays physically at head (head only moves on push). */
    for (int j = i; j < s->count - 1; j++) {
        int a = (s->head + j) % WF_NOTIF_MAX;
        int b = (s->head + j + 1) % WF_NOTIF_MAX;
        s->items[a] = s->items[b];
    }
    s->count--;
    return s->count;
}

void wf_notif_clear(wf_notif_store_t *s)
{
    s->count = 0;
    s->head = 0;
    /* next_id intentionally preserved so ids stay unique across clears. */
}

void wf_notif_reltime(char *buf, size_t n, long now, long ts)
{
    long d = now - ts;
    if (d < 60)          snprintf(buf, n, "now");
    else if (d < 3600)   snprintf(buf, n, "%ldm", d / 60);
    else if (d < 86400)  snprintf(buf, n, "%ldh", d / 3600);
    else                 snprintf(buf, n, "%ldj", d / 86400);
}
