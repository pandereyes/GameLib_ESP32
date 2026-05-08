#include "gamelib.h"
#include "font_8x8.h"

static void draw_char(gamelib_t *g, int x, int y, char ch, gamelib_color_t c, int sw, int sh)
{
    const uint8_t *glyph = font_8x8[(unsigned char)ch];
    framebuffer_t *fb = &g->fb;
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                int px = x + col * sw;
                int py = y + row * sh;
                for (int sy = 0; sy < sh; sy++) {
                    for (int sx = 0; sx < sw; sx++) {
                        int fx = px + sx;
                        int fy = py + sy;
                        if (fx >= fb->clip_x && fx < fb->clip_x + fb->clip_w &&
                            fy >= fb->clip_y && fy < fb->clip_y + fb->clip_h) {
                            fb->pixels[fy * fb->width + fx] = c;
                        }
                    }
                }
            }
        }
    }
}

void gamelib_draw_text(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c)
{
    while (*s) {
        draw_char(g, x, y, *s, c, 1, 1);
        x += 8;
        s++;
    }
}

void gamelib_draw_text_scale(gamelib_t *g, int x, int y, const char *s,
                             gamelib_color_t c, int scale_w, int scale_h)
{
    if (scale_w < 1) scale_w = 1;
    if (scale_h < 1) scale_h = 1;
    while (*s) {
        draw_char(g, x, y, *s, c, scale_w, scale_h);
        x += 8 * scale_w;
        s++;
    }
}

void gamelib_draw_number(gamelib_t *g, int x, int y, int n, gamelib_color_t c)
{
    char buf[16];
    int i = 0;
    if (n < 0) {
        buf[i++] = '-';
        n = -n;
    }
    if (n == 0) {
        buf[i++] = '0';
    } else {
        int start = i;
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        for (int j = start, k = i - 1; j < k; j++, k--) {
            char t = buf[j];
            buf[j] = buf[k];
            buf[k] = t;
        }
    }
    buf[i] = '\0';
    gamelib_draw_text(g, x, y, buf, c);
}
