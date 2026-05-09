#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

static void fb_set_pixel(framebuffer_t *fb, int x, int y, gamelib_color_t c)
{
    if (x >= fb->clip_x && x < fb->clip_x + fb->clip_w &&
        y >= fb->clip_y && y < fb->clip_y + fb->clip_h) {
        fb->pixels[y * fb->width + x] = c;
    }
}

static void fb_hline(framebuffer_t *fb, int x1, int x2, int y, gamelib_color_t c)
{
    if (y < fb->clip_y || y >= fb->clip_y + fb->clip_h) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < fb->clip_x) x1 = fb->clip_x;
    if (x2 >= fb->clip_x + fb->clip_w) x2 = fb->clip_x + fb->clip_w - 1;
    if (x1 > x2) return;
    gamelib_color_t *row = fb->pixels + y * fb->width;
    for (int x = x1; x <= x2; x++) row[x] = c;
}

void gamelib_draw_line(gamelib_t *g, int x1, int y1, int x2, int y2, gamelib_color_t c)
{
    if (g->fb.clip_w <= 0 || g->fb.clip_h <= 0) return;
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        fb_set_pixel(&g->fb, x1, y1, c);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void gamelib_draw_rect(gamelib_t *g, int x, int y, int w, int h, gamelib_color_t c)
{
    if (w <= 0 || h <= 0) return;
    fb_hline(&g->fb, x, x + w - 1, y, c);
    fb_hline(&g->fb, x, x + w - 1, y + h - 1, c);
    for (int j = y + 1; j < y + h - 1; j++) {
        fb_set_pixel(&g->fb, x, j, c);
        fb_set_pixel(&g->fb, x + w - 1, j, c);
    }
}

void gamelib_fill_rect(gamelib_t *g, int x, int y, int w, int h, gamelib_color_t c)
{
    if (w <= 0 || h <= 0) return;
    int x1 = x, y1 = y, x2 = x + w, y2 = y + h;
    if (x1 < g->fb.clip_x) x1 = g->fb.clip_x;
    if (y1 < g->fb.clip_y) y1 = g->fb.clip_y;
    if (x2 > g->fb.clip_x + g->fb.clip_w) x2 = g->fb.clip_x + g->fb.clip_w;
    if (y2 > g->fb.clip_y + g->fb.clip_h) y2 = g->fb.clip_y + g->fb.clip_h;
    for (int j = y1; j < y2; j++) {
        fb_hline(&g->fb, x1, x2 - 1, j, c);
    }
}

void gamelib_draw_circle(gamelib_t *g, int cx, int cy, int r, gamelib_color_t c)
{
    if (r < 0) return;
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        fb_set_pixel(&g->fb, cx + x, cy + y, c);
        fb_set_pixel(&g->fb, cx + x, cy - y, c);
        fb_set_pixel(&g->fb, cx - x, cy + y, c);
        fb_set_pixel(&g->fb, cx - x, cy - y, c);
        fb_set_pixel(&g->fb, cx + y, cy + x, c);
        fb_set_pixel(&g->fb, cx + y, cy - x, c);
        fb_set_pixel(&g->fb, cx - y, cy + x, c);
        fb_set_pixel(&g->fb, cx - y, cy - x, c);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void gamelib_fill_circle(gamelib_t *g, int cx, int cy, int r, gamelib_color_t c)
{
    gamelib_fill_ellipse(g, cx, cy, r, r, c);
}

void gamelib_draw_ellipse(gamelib_t *g, int cx, int cy, int rx, int ry, gamelib_color_t c)
{
    if (rx < 0 || ry < 0) return;
    if (rx == 0 && ry == 0) {
        fb_set_pixel(&g->fb, cx, cy, c);
        return;
    }
    if (rx == 0) {
        gamelib_draw_line(g, cx, cy - ry, cx, cy + ry, c);
        return;
    }
    if (ry == 0) {
        gamelib_draw_line(g, cx - rx, cy, cx + rx, cy, c);
        return;
    }
    int rx2 = rx * rx, ry2 = ry * ry;
    int twoRx2 = 2 * rx2, twoRy2 = 2 * ry2;
    int x = 0, y = ry, px = 0, py = twoRx2 * y;
    double p = (double)ry2 - (double)rx2 * ry + 0.25 * rx2;
    while (px < py) {
        fb_set_pixel(&g->fb, cx + x, cy + y, c);
        fb_set_pixel(&g->fb, cx - x, cy + y, c);
        fb_set_pixel(&g->fb, cx + x, cy - y, c);
        fb_set_pixel(&g->fb, cx - x, cy - y, c);
        x++; px += twoRy2;
        if (p < 0.0) {
            p += ry2 + px;
        } else {
            y--; py -= twoRx2;
            p += ry2 + px - py;
        }
    }
    p = (double)ry2 * (x + 0.5) * (x + 0.5) +
        (double)rx2 * (y - 1.0) * (y - 1.0) -
        (double)rx2 * ry2;
    while (y >= 0) {
        fb_set_pixel(&g->fb, cx + x, cy + y, c);
        fb_set_pixel(&g->fb, cx - x, cy + y, c);
        fb_set_pixel(&g->fb, cx + x, cy - y, c);
        fb_set_pixel(&g->fb, cx - x, cy - y, c);
        y--; py -= twoRx2;
        if (p > 0.0) {
            p += rx2 - py;
        } else {
            x++; px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void gamelib_fill_ellipse(gamelib_t *g, int cx, int cy, int rx, int ry, gamelib_color_t c)
{
    if (rx < 0 || ry < 0) return;
    if (rx == 0 && ry == 0) {
        fb_set_pixel(&g->fb, cx, cy, c);
        return;
    }
    if (rx == 0) {
        gamelib_draw_line(g, cx, cy - ry, cx, cy + ry, c);
        return;
    }
    if (ry == 0) {
        gamelib_draw_line(g, cx - rx, cy, cx + rx, cy, c);
        return;
    }
    int rx2 = rx * rx, ry2 = ry * ry;
    int twoRx2 = 2 * rx2, twoRy2 = 2 * ry2;
    int x = 0, y = ry, px = 0, py = twoRx2 * y;
    double p = (double)ry2 - (double)rx2 * ry + 0.25 * rx2;
    while (px < py) {
        fb_hline(&g->fb, cx - x, cx + x, cy + y, c);
        if (y > 0) fb_hline(&g->fb, cx - x, cx + x, cy - y, c);
        x++; px += twoRy2;
        if (p < 0.0) {
            p += ry2 + px;
        } else {
            y--; py -= twoRx2;
            p += ry2 + px - py;
        }
    }
    p = (double)ry2 * (x + 0.5) * (x + 0.5) +
        (double)rx2 * (y - 1.0) * (y - 1.0) -
        (double)rx2 * ry2;
    while (y >= 0) {
        fb_hline(&g->fb, cx - x, cx + x, cy + y, c);
        if (y > 0) fb_hline(&g->fb, cx - x, cx + x, cy - y, c);
        y--; py -= twoRx2;
        if (p > 0.0) {
            p += rx2 - py;
        } else {
            x++; px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void gamelib_draw_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c)
{
    gamelib_draw_line(g, x1, y1, x2, y2, c);
    gamelib_draw_line(g, x2, y2, x3, y3, c);
    gamelib_draw_line(g, x3, y3, x1, y1, c);
}

void gamelib_fill_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c)
{
    if (y1 > y2) { int t; t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    if (y1 > y3) { int t; t = x1; x1 = x3; x3 = t; t = y1; y1 = y3; y3 = t; }
    if (y2 > y3) { int t; t = x2; x2 = x3; x3 = t; t = y2; y2 = y3; y3 = t; }
    if (y2 == y3) {
        if (x2 > x3) { int t = x2; x2 = x3; x3 = t; }
        for (int y = y1; y <= y3; y++) {
            float t = (y3 == y1) ? 0.0f : (float)(y - y1) / (y3 - y1);
            int xL = x1 + (int)(t * (x2 - x1));
            int xR = x1 + (int)(t * (x3 - x1));
            if (xL > xR) { int tmp = xL; xL = xR; xR = tmp; }
            fb_hline(&g->fb, xL, xR, y, c);
        }
    } else {
        for (int y = y1; y <= y2; y++) {
            float t = (y2 == y1) ? 0.0f : (float)(y - y1) / (y2 - y1);
            int xL = x1 + (int)(t * (x2 - x1));
            int xR = x1 + (int)(t * (x3 - x1));
            if (xL > xR) { int tmp = xL; xL = xR; xR = tmp; }
            fb_hline(&g->fb, xL, xR, y, c);
        }
        for (int y = y2; y <= y3; y++) {
            float t = (y3 == y2) ? 0.0f : (float)(y - y2) / (y3 - y2);
            int xL = x2 + (int)(t * (x3 - x2));
            int xR = x1 + (int)((float)(y - y1) / (y3 - y1) * (x3 - x1));
            if (xL > xR) { int tmp = xL; xL = xR; xR = tmp; }
            fb_hline(&g->fb, xL, xR, y, c);
        }
    }
}
