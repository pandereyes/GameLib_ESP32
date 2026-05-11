#define STB_TRUETYPE_IMPLEMENTATION
#include "font.h"
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern font_t *g_fonts;

static font_t* font_get(int font_id)
{
    if (!g_fonts || font_id < 0 || font_id >= MAX_FONTS) return NULL;
    if (!g_fonts[font_id].used) return NULL;
    return &g_fonts[font_id];
}

int gamelib_font_load(gamelib_t *g, const uint8_t *ttf_data, int font_size)
{
    (void)g;
    if (!g_fonts || !ttf_data || font_size <= 0) return -1;
    for (int i = 0; i < MAX_FONTS; i++) {
        if (!g_fonts[i].used) {
            font_t *f = &g_fonts[i];
            memset(f, 0, sizeof(*f));
            f->ttf_buffer = ttf_data;
            if (!stbtt_InitFont(&f->info, f->ttf_buffer, stbtt_GetFontOffsetForIndex(f->ttf_buffer, 0)))
                return -1;
            f->font_size = font_size;
            f->scale = stbtt_ScaleForPixelHeight(&f->info, (float)font_size);
            int ascent, descent, linegap;
            stbtt_GetFontVMetrics(&f->info, &ascent, &descent, &linegap);
            f->ascent = (int)(ascent * f->scale);
            f->descent = (int)(descent * f->scale);
            f->used = true;
            return i;
        }
    }
    return -1;
}

void gamelib_font_free(gamelib_t *g, int font_id)
{
    (void)g;
    font_t *f = font_get(font_id);
    if (!f) return;
    for (int i = 0; i < 256; i++) {
        free(f->glyph_cache[i]);
        f->glyph_cache[i] = NULL;
    }
    memset(f, 0, sizeof(*f));
}

void gamelib_draw_text_font(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c, int font_id)
{
    font_t *f = font_get(font_id);
    if (!f || !s) return;
    int pen_x = x;
    while (*s) {
        int ch = (unsigned char)*s;
        if (!f->glyph_cache[ch]) {
            int w, h, xoff, yoff;
            unsigned char *bmp = stbtt_GetCodepointBitmap(&f->info, 0, f->scale,
                                                          ch, &w, &h, &xoff, &yoff);
            f->glyph_cache[ch] = bmp;
        }
        uint8_t *bmp = f->glyph_cache[ch];
        if (bmp) {
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&f->info, ch, f->scale, f->scale, &x0, &y0, &x1, &y1);
            int bw = x1 - x0, bh = y1 - y0;
            for (int py = 0; py < bh; py++) {
                for (int px = 0; px < bw; px++) {
                    uint8_t alpha = bmp[py * bw + px];
                    if (alpha > 0) {
                        gamelib_set_pixel(g, pen_x + x0 + px, y + f->ascent + y0 + py, c);
                    }
                }
            }
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&f->info, ch, &adv, &lsb);
            pen_x += (int)(adv * f->scale);
        }
        s++;
    }
}

int gamelib_get_text_width_font(gamelib_t *g, const char *s, int font_id)
{
    (void)g;
    font_t *f = font_get(font_id);
    if (!f || !s) return 0;
    int width = 0;
    while (*s) {
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&f->info, (unsigned char)*s, &adv, &lsb);
        width += (int)(adv * f->scale);
        s++;
    }
    return width;
}

void gamelib_draw_printf_font(gamelib_t *g, int x, int y, gamelib_color_t c, int font_id, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gamelib_draw_text_font(g, x, y, buf, c, font_id);
}
