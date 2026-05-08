#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

static inline void fb_set_pixel(framebuffer_t *fb, int x, int y, gamelib_color_t c)
{
    if (x >= fb->clip_x && x < fb->clip_x + fb->clip_w &&
        y >= fb->clip_y && y < fb->clip_y + fb->clip_h) {
        fb->pixels[y * fb->width + x] = c;
    }
}

void gamelib_clear(gamelib_t *g, gamelib_color_t color)
{
    int total = g->fb.width * g->fb.height;
    for (int i = 0; i < total; i++) {
        g->fb.pixels[i] = color;
    }
}

void gamelib_set_pixel(gamelib_t *g, int x, int y, gamelib_color_t c)
{
    fb_set_pixel(&g->fb, x, y, c);
}

void gamelib_set_clip(gamelib_t *g, int x, int y, int w, int h)
{
    g->fb.clip_x = x;
    g->fb.clip_y = y;
    g->fb.clip_w = w;
    g->fb.clip_h = h;
}

void gamelib_clear_clip(gamelib_t *g)
{
    g->fb.clip_x = 0;
    g->fb.clip_y = 0;
    g->fb.clip_w = g->fb.width;
    g->fb.clip_h = g->fb.height;
}

static void fb_hline(framebuffer_t *fb, int x1, int x2, int y, gamelib_color_t c)
{
    if (y < fb->clip_y || y >= fb->clip_y + fb->clip_h) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < fb->clip_x) x1 = fb->clip_x;
    if (x2 >= fb->clip_x + fb->clip_w) x2 = fb->clip_x + fb->clip_w - 1;
    gamelib_color_t *row = fb->pixels + y * fb->width;
    for (int x = x1; x <= x2; x++) {
        row[x] = c;
    }
}
