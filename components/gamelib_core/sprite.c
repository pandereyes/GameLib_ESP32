#include "gamelib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

int gamelib_sprite_create(gamelib_t *g, int w, int h)
{
    for (int i = 0; i < MAX_SPRITES; i++) {
        if (!g->sprites[i].used) {
            g->sprites[i].pixels = (gamelib_color_t*)calloc(w * h, sizeof(gamelib_color_t));
            if (!g->sprites[i].pixels) return -1;
            g->sprites[i].width = w;
            g->sprites[i].height = h;
            g->sprites[i].used = true;
            g->sprites[i].has_color_key = false;
            g->sprites[i].color_key = COLOR_NONE;
            return i;
        }
    }
    return -1;
}

void gamelib_sprite_free(gamelib_t *g, int id)
{
    if (id < 0 || id >= MAX_SPRITES) return;
    if (g->sprites[id].used) {
        free(g->sprites[id].pixels);
        g->sprites[id].pixels = NULL;
        g->sprites[id].used = false;
    }
}

void gamelib_sprite_set_pixel(gamelib_t *g, int id, int x, int y, gamelib_color_t c)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    if (x < 0 || x >= g->sprites[id].width || y < 0 || y >= g->sprites[id].height) return;
    g->sprites[id].pixels[y * g->sprites[id].width + x] = c;
}

int gamelib_sprite_width(gamelib_t *g, int id)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return 0;
    return g->sprites[id].width;
}

int gamelib_sprite_height(gamelib_t *g, int id)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return 0;
    return g->sprites[id].height;
}

void gamelib_sprite_set_color_key(gamelib_t *g, int id, gamelib_color_t c)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    g->sprites[id].color_key = c;
    g->sprites[id].has_color_key = true;
}

/* gamelib_draw_sprite_region defined before gamelib_draw_sprite to avoid forward declaration */
void gamelib_draw_sprite_region(gamelib_t *g, int id, int dst_x, int dst_y,
                                int sx, int sy, int sw, int sh)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx + sw > sp->width)  sw = sp->width - sx;
    if (sy + sh > sp->height) sh = sp->height - sy;
    framebuffer_t *fb = &g->fb;
    for (int row = 0; row < sh; row++) {
        for (int col = 0; col < sw; col++) {
            gamelib_color_t c = sp->pixels[(sy + row) * sp->width + (sx + col)];
            if (sp->has_color_key && c == sp->color_key) continue;
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}

void gamelib_draw_sprite(gamelib_t *g, int id, int x, int y)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    gamelib_draw_sprite_region(g, id, x, y, 0, 0,
                               g->sprites[id].width, g->sprites[id].height);
}

void gamelib_draw_sprite_ex(gamelib_t *g, int id, int dst_x, int dst_y, int flags)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    int sw = sp->width;
    int sh = sp->height;
    framebuffer_t *fb = &g->fb;

    for (int row = 0; row < sh; row++) {
        for (int col = 0; col < sw; col++) {
            int src_x = (flags & SPRITE_FLIP_H) ? (sw - 1 - col) : col;
            int src_y = (flags & SPRITE_FLIP_V) ? (sh - 1 - row) : row;
            gamelib_color_t c = sp->pixels[src_y * sw + src_x];
            if ((flags & SPRITE_COLORKEY) && sp->has_color_key && c == sp->color_key) continue;
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}

void gamelib_draw_sprite_scaled(gamelib_t *g, int id, int dst_x, int dst_y, int dst_w, int dst_h)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    int sw = sp->width;
    int sh = sp->height;
    if (dst_w <= 0 || dst_h <= 0) return;
    framebuffer_t *fb = &g->fb;

    for (int row = 0; row < dst_h; row++) {
        int src_y = row * sh / dst_h;
        for (int col = 0; col < dst_w; col++) {
            int src_x = col * sw / dst_w;
            gamelib_color_t c = sp->pixels[src_y * sw + src_x];
            if (sp->has_color_key && c == sp->color_key) continue;
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}

void gamelib_draw_sprite_frame_scaled(gamelib_t *g, int id, int dst_x, int dst_y,
                                       int fw, int fh, int frame, int dst_w, int dst_h,
                                       int flags)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    if (dst_w <= 0 || dst_h <= 0 || fw <= 0 || fh <= 0) return;

    int sheet_w = sp->width;
    int sheet_h = sp->height;
    int src_ox = frame * fw;
    if (src_ox + fw > sheet_w || fh > sheet_h) return;
    framebuffer_t *fb = &g->fb;

    for (int row = 0; row < dst_h; row++) {
        int fy = row * fh / dst_h;
        int src_y = (flags & SPRITE_FLIP_V) ? (fh - 1 - fy) : fy;
        for (int col = 0; col < dst_w; col++) {
            int fx = col * fw / dst_w;
            int src_x = src_ox + ((flags & SPRITE_FLIP_H) ? (fw - 1 - fx) : fx);
            gamelib_color_t c = sp->pixels[src_y * sheet_w + src_x];
            if ((flags & SPRITE_COLORKEY) && sp->has_color_key && c == sp->color_key) continue;
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}

void gamelib_draw_sprite_rotated(gamelib_t *g, int id, int cx, int cy,
                                  double angle, int flags)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    int sw = sp->width, sh = sp->height;
    framebuffer_t *fb = &g->fb;

    double rad = angle * 3.141592653589793 / 180.0;
    double ca = cos(rad), sa = sin(rad);

    double hsw = sw * 0.5, hsh = sh * 0.5;
    double corners[4][2] = {{-hsw, -hsh}, {hsw, -hsh}, {hsw, hsh}, {-hsw, hsh}};
    int min_x = 9999, max_x = -9999, min_y = 9999, max_y = -9999;
    for (int i = 0; i < 4; i++) {
        double rx = corners[i][0] * ca - corners[i][1] * sa;
        double ry = corners[i][0] * sa + corners[i][1] * ca;
        if ((int)rx < min_x) min_x = (int)rx;
        if ((int)rx > max_x) max_x = (int)rx;
        if ((int)ry < min_y) min_y = (int)ry;
        if ((int)ry > max_y) max_y = (int)ry;
    }

    for (int dy = min_y; dy <= max_y; dy++) {
        for (int dx = min_x; dx <= max_x; dx++) {
            double src_x =  dx * ca + dy * sa + hsw;
            double src_y = -dx * sa + dy * ca + hsh;
            int sx = (int)src_x, sy = (int)src_y;
            if (sx < 0 || sx >= sw || sy < 0 || sy >= sh) continue;

            if (flags & SPRITE_FLIP_H) sx = sw - 1 - sx;
            if (flags & SPRITE_FLIP_V) sy = sh - 1 - sy;

            gamelib_color_t c = sp->pixels[sy * sw + sx];
            if ((flags & SPRITE_COLORKEY) && sp->has_color_key && c == sp->color_key) continue;
            int px = cx + dx, py = cy + dy;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}

void gamelib_draw_sprite_frame_rotated(gamelib_t *g, int id, int cx, int cy,
                                        int fw, int fh, int frame, double angle, int flags)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    int sheet_w = sp->width, sheet_h = sp->height;
    int src_ox = frame * fw;
    if (src_ox + fw > sheet_w || fh > sheet_h) return;
    framebuffer_t *fb = &g->fb;

    double rad = angle * 3.141592653589793 / 180.0;
    double ca = cos(rad), sa = sin(rad);

    double hfw = fw * 0.5, hfh = fh * 0.5;
    double corners[4][2] = {{-hfw, -hfh}, {hfw, -hfh}, {hfw, hfh}, {-hfw, hfh}};
    int min_x = 9999, max_x = -9999, min_y = 9999, max_y = -9999;
    for (int i = 0; i < 4; i++) {
        double rx = corners[i][0] * ca - corners[i][1] * sa;
        double ry = corners[i][0] * sa + corners[i][1] * ca;
        if ((int)rx < min_x) min_x = (int)rx;
        if ((int)rx > max_x) max_x = (int)rx;
        if ((int)ry < min_y) min_y = (int)ry;
        if ((int)ry > max_y) max_y = (int)ry;
    }

    for (int dy = min_y; dy <= max_y; dy++) {
        for (int dx = min_x; dx <= max_x; dx++) {
            double fx =  dx * ca + dy * sa + hfw;
            double fy = -dx * sa + dy * ca + hfh;
            int sx = (int)fx, sy = (int)fy;
            if (sx < 0 || sx >= fw || sy < 0 || sy >= fh) continue;

            if (flags & SPRITE_FLIP_H) sx = fw - 1 - sx;
            if (flags & SPRITE_FLIP_V) sy = fh - 1 - sy;

            gamelib_color_t c = sp->pixels[sy * sheet_w + (src_ox + sx)];
            if ((flags & SPRITE_COLORKEY) && sp->has_color_key && c == sp->color_key) continue;
            int px = cx + dx, py = cy + dy;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}
