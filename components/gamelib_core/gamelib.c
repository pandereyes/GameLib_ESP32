#include "gamelib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

gamelib_hal_t g_hal;
tilemap_t *g_tilemaps = NULL;
font_t    *g_fonts = NULL;

extern const tileprop_desc_t tileprop_table[];
extern const int             tileprop_table_size;

int gamelib_init(gamelib_t *g, int w, int h, int target_fps)
{
    memset(g, 0, sizeof(*g));
    g->fb.width = w;
    g->fb.height = h;
    g->fb.clip_x = 0;
    g->fb.clip_y = 0;
    g->fb.clip_w = w;
    g->fb.clip_h = h;
    g->target_fps = target_fps;

    size_t fb_size = w * h * sizeof(gamelib_color_t);
    if (g_hal.display.alloc_fb) {
        g->fb.pixels = (gamelib_color_t*)g_hal.display.alloc_fb(fb_size);
    } else {
        g->fb.pixels = (gamelib_color_t*)calloc(1, fb_size);
    }
    if (!g->fb.pixels) return -1;

    if (g_hal.display.init) {
        if (g_hal.display.init() != 0) {
            if (g_hal.display.free_fb) g_hal.display.free_fb(g->fb.pixels);
            else free(g->fb.pixels);
            return -1;
        }
    }
    if (g_hal.input.init)  g_hal.input.init();
    if (g_hal.timer.init)  g_hal.timer.init();
    if (g_hal.audio.init)  g_hal.audio.init();
    if (g_hal.fs.init)     g_hal.fs.init();

    g_tilemaps = (tilemap_t*)calloc(MAX_TILEMAPS, sizeof(tilemap_t));
    g_fonts = (font_t*)calloc(MAX_FONTS, sizeof(font_t));

    g->frame_start = (double)g_hal.timer.micros() / 1000000.0;
    g->start_time  = g->frame_start;
    g->running = true;
    return 0;
}

void gamelib_deinit(gamelib_t *g)
{
    if (g_tilemaps) {
        for (int i = 0; i < MAX_TILEMAPS; i++) {
            if (g_tilemaps[i].used) {
                free(g_tilemaps[i].tiles);
                g_tilemaps[i].tiles = NULL;
            }
        }
        free(g_tilemaps);
        g_tilemaps = NULL;
    }
    if (g_fonts) {
        for (int i = 0; i < MAX_FONTS; i++) {
            if (g_fonts[i].used) {
                for (int j = 0; j < 256; j++) {
                    free(g_fonts[i].glyph_cache[j]);
                    g_fonts[i].glyph_cache[j] = NULL;
                }
            }
        }
        free(g_fonts);
        g_fonts = NULL;
    }
    if (g_hal.display.deinit) g_hal.display.deinit();
    if (g_hal.fs.deinit)      g_hal.fs.deinit();
    if (g_hal.audio.deinit)   g_hal.audio.deinit();
    for (int i = 0; i < MAX_SPRITES; i++) {
        gamelib_sprite_free(g, i);
    }
    if (g_hal.display.free_fb) g_hal.display.free_fb(g->fb.pixels);
    else free(g->fb.pixels);
    g->fb.pixels = NULL;
    g->running = false;
}

bool gamelib_is_closed(gamelib_t *g)
{
    return !g->running;
}

void gamelib_update(gamelib_t *g)
{
    if (!g->running) return;

    /* flush */
    if (g_hal.display.flush) {
        g_hal.display.flush(g->fb.pixels, 0, 0, g->fb.width, g->fb.height);
    }

    /* input */
    if (g_hal.input.poll) {
        g_hal.input.poll();
        double now = gamelib_get_time(g);
        for (int i = 0; i < KEY_COUNT; i++) {
            g->key_prev[i] = g->keystate[i];
            
            uint8_t raw_state = g_hal.input.is_down((hal_key_t)i) ? 1 : 0;
            if (raw_state != g->keystate[i]) {
                if (now - g->key_time[i] > 0.05) { /* 50ms 消抖 */
                    g->keystate[i] = raw_state;
                    g->key_time[i] = now;
                }
            } else {
                g->key_time[i] = now; /* 只要没变，就刷新稳定时间 */
            }
        }
        g->mouse_x = g_hal.input.mouse_x ? g_hal.input.mouse_x() : 0;
        g->mouse_y = g_hal.input.mouse_y ? g_hal.input.mouse_y() : 0;
    }
}

void gamelib_wait_frame(gamelib_t *g)
{
    if (!g->running) return;
    double now = (double)g_hal.timer.micros() / 1000000.0;
    g->delta_time = now - g->frame_start;
    double target = 1.0 / (double)g->target_fps;
    if (g->delta_time < target) {
        uint32_t sleep_ms = (uint32_t)((target - g->delta_time) * 1000.0);
        g_hal.timer.sleep_ms(sleep_ms);
        now = (double)g_hal.timer.micros() / 1000000.0;
        g->delta_time = now - g->frame_start;
    }
    if (g->delta_time > 0.0) {
        g->fps = 1.0 / g->delta_time;
    }
    g->frame_start = now;
}

double gamelib_get_fps(gamelib_t *g)
{
    return g->fps;
}

double gamelib_get_time(gamelib_t *g)
{
    double now = (double)g_hal.timer.micros() / 1000000.0;
    return now - g->start_time;
}

double gamelib_get_delta_time(gamelib_t *g)
{
    return g->delta_time;
}

int gamelib_get_width(gamelib_t *g)
{
    return g->fb.width;
}

int gamelib_get_height(gamelib_t *g)
{
    return g->fb.height;
}

void gamelib_animator_update(gamelib_t *g, gamelib_animator_t *a)
{
    if (!g || !a) return;

    double now_s = gamelib_get_time(g);
    uint32_t now_ms = (now_s > 0.0) ? (uint32_t)(now_s * 1000.0) : 0;
    if (!a->has_time_sample) {
        a->last_time_ms = now_ms;
        a->has_time_sample = true;
        a->frame_changed = false;
        return;
    }

    uint32_t delta_ms = now_ms - a->last_time_ms;
    a->last_time_ms = now_ms;
    gamelib_animator_update_ms(a, delta_ms);
}

/* --- input --- */
bool gamelib_is_key_down(gamelib_t *g, int key)
{
    if (key < 0 || key >= KEY_COUNT) return false;
    return g->keystate[key] != 0;
}

bool gamelib_is_key_pressed(gamelib_t *g, int key)
{
    if (key < 0 || key >= KEY_COUNT) return false;
    return g->keystate[key] != 0 && g->key_prev[key] == 0;
}

bool gamelib_is_key_released(gamelib_t *g, int key)
{
    if (key < 0 || key >= KEY_COUNT) return false;
    return g->keystate[key] == 0 && g->key_prev[key] != 0;
}

int gamelib_mouse_x(gamelib_t *g) { return g->mouse_x; }
int gamelib_mouse_y(gamelib_t *g) { return g->mouse_y; }

void gamelib_draw_printf(gamelib_t *g, int x, int y, gamelib_color_t c, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gamelib_draw_text(g, x, y, buf, c);
}

/* --- helpers --- */
int gamelib_random(int minVal, int maxVal)
{
    return minVal + (rand() % (maxVal - minVal + 1));
}

bool gamelib_rect_overlap(int x1,int y1,int w1,int h1, int x2,int y2,int w2,int h2)
{
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

bool gamelib_point_in_rect(int px,int py, int x,int y,int w,int h)
{
    return (px >= x && px < x + w && py >= y && py < y + h);
}

float gamelib_distance_f(int x1,int y1, int x2,int y2)
{
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    return sqrtf(dx * dx + dy * dy);
}

/* --- grid --- */
void gamelib_draw_grid(gamelib_t *g, int ox, int oy, int rows, int cols, int cell_size, gamelib_color_t c)
{
    for (int i = 0; i <= cols; i++)
        gamelib_draw_line(g, ox + i * cell_size, oy, ox + i * cell_size, oy + rows * cell_size, c);
    for (int i = 0; i <= rows; i++)
        gamelib_draw_line(g, ox, oy + i * cell_size, ox + cols * cell_size, oy + i * cell_size, c);
}

void gamelib_fill_cell(gamelib_t *g, int ox, int oy, int row, int col, int cell_size, gamelib_color_t c)
{
    gamelib_fill_rect(g, ox + col * cell_size, oy + row * cell_size, cell_size, cell_size, c);
}

/* --- clip query --- */
int gamelib_get_clip_x(gamelib_t *g) { return g->fb.clip_x; }
int gamelib_get_clip_y(gamelib_t *g) { return g->fb.clip_y; }
int gamelib_get_clip_w(gamelib_t *g) { return g->fb.clip_w; }
int gamelib_get_clip_h(gamelib_t *g) { return g->fb.clip_h; }

/* --- file i/o --- */
void* gamelib_file_open(gamelib_t *g, const char *path, const char *mode)
{
    (void)g;
    if (!g_hal.fs.open || !path || !mode) return NULL;
    return g_hal.fs.open(path, mode);
}

int gamelib_file_read(gamelib_t *g, void *file, uint8_t *buf, int len)
{
    (void)g;
    if (!g_hal.fs.read || !file || !buf || len <= 0) return -1;
    return g_hal.fs.read(file, buf, len);
}

int gamelib_file_write(gamelib_t *g, void *file, const uint8_t *buf, int len)
{
    (void)g;
    if (!g_hal.fs.write || !file || !buf || len <= 0) return -1;
    return g_hal.fs.write(file, buf, len);
}

int gamelib_file_seek(gamelib_t *g, void *file, int offset, int whence)
{
    (void)g;
    if (!g_hal.fs.seek || !file) return -1;
    return g_hal.fs.seek(file, offset, whence);
}

int gamelib_file_tell(gamelib_t *g, void *file)
{
    (void)g;
    if (!g_hal.fs.tell || !file) return -1;
    return g_hal.fs.tell(file);
}

int gamelib_file_size(gamelib_t *g, void *file)
{
    (void)g;
    if (!g_hal.fs.size || !file) return -1;
    return g_hal.fs.size(file);
}

int gamelib_file_close(gamelib_t *g, void *file)
{
    (void)g;
    if (!g_hal.fs.close || !file) return -1;
    return g_hal.fs.close(file);
}

/* --- tile property query --- */
bool gamelib_tile_prop_bool(int firstgid, const char *name, int tile_gid)
{
    int local_id = tile_gid - firstgid;
    for (int i = 0; i < tileprop_table_size; i++) {
        const tileprop_desc_t *d = &tileprop_table[i];
        if (d->firstgid == firstgid && d->type == TILEPROP_TYPE_BOOL &&
            strcmp(d->name, name) == 0) {
            if (local_id < 0 || local_id >= d->count * 32) return false;
            const uint32_t *bits = (const uint32_t*)d->data;
            return (bits[local_id >> 5] >> (local_id & 0x1F)) & 1;
        }
    }
    return false;
}

int gamelib_tile_prop_int(int firstgid, const char *name, int tile_gid)
{
    int local_id = tile_gid - firstgid;
    for (int i = 0; i < tileprop_table_size; i++) {
        const tileprop_desc_t *d = &tileprop_table[i];
        if (d->firstgid == firstgid && d->type == TILEPROP_TYPE_INT &&
            strcmp(d->name, name) == 0) {
            if (local_id < 0 || local_id >= d->count) return 0;
            const int32_t *arr = (const int32_t*)d->data;
            return arr[local_id];
        }
    }
    return 0;
}

float gamelib_tile_prop_float(int firstgid, const char *name, int tile_gid)
{
    int local_id = tile_gid - firstgid;
    for (int i = 0; i < tileprop_table_size; i++) {
        const tileprop_desc_t *d = &tileprop_table[i];
        if (d->firstgid == firstgid && d->type == TILEPROP_TYPE_FLOAT &&
            strcmp(d->name, name) == 0) {
            if (local_id < 0 || local_id >= d->count) return 0.0f;
            const float *arr = (const float*)d->data;
            return arr[local_id];
        }
    }
    return 0.0f;
}
