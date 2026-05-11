#ifndef GAMELIB_H
#define GAMELIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"
#include "hal_display.h"
#include "hal_input.h"
#include "hal_audio.h"
#include "hal_timer.h"
#include "tilemap.h"
#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SPRITES 64
#define MAX_TILEMAPS 8
#define SPRITE_COLORKEY 0x04

/* --- sprite flip flags --- */
#define SPRITE_FLIP_H  1
#define SPRITE_FLIP_V  2

/* --- sprite --- */
typedef struct {
    gamelib_color_t *pixels;
    int  width;
    int  height;
    bool used;
    gamelib_color_t color_key;
    bool has_color_key;
} sprite_t;

/* --- framebuffer --- */
typedef struct {
    gamelib_color_t *pixels;
    int width;
    int height;
    int clip_x, clip_y;
    int clip_w, clip_h;
} framebuffer_t;

/* --- main context --- */
typedef struct {
    framebuffer_t fb;
    bool          running;
    double        delta_time;
    double        fps;
    double        frame_start;
    double        start_time;    /* init 时刻时间戳(秒) */
    int           target_fps;

    sprite_t      sprites[MAX_SPRITES];
    tilemap_t     tilemaps[MAX_TILEMAPS];
    font_t        fonts[MAX_FONTS];

    uint8_t       keystate[KEY_COUNT];
    uint8_t       key_prev[KEY_COUNT];
    int           mouse_x;
    int           mouse_y;
} gamelib_t;

/* --- HAL binding --- */
typedef struct {
    hal_display_t display;
    hal_input_t   input;
    hal_audio_t   audio;
    hal_timer_t   timer;
} gamelib_hal_t;

extern gamelib_hal_t g_hal;
extern tilemap_t *g_tilemaps;
extern font_t    *g_fonts;

/* --- lifecycle --- */
int    gamelib_init(gamelib_t *g, int w, int h, int target_fps);
void   gamelib_deinit(gamelib_t *g);
bool   gamelib_is_closed(gamelib_t *g);
void   gamelib_update(gamelib_t *g);
void   gamelib_wait_frame(gamelib_t *g);
double gamelib_get_fps(gamelib_t *g);

/* --- time --- */
double gamelib_get_time(gamelib_t *g);
double gamelib_get_delta_time(gamelib_t *g);
int    gamelib_get_width(gamelib_t *g);
int    gamelib_get_height(gamelib_t *g);

/* --- drawing --- */
void gamelib_clear(gamelib_t *g, gamelib_color_t color);
void gamelib_set_pixel(gamelib_t *g, int x, int y, gamelib_color_t c);
void gamelib_draw_line(gamelib_t *g, int x1,int y1,int x2,int y2, gamelib_color_t c);
void gamelib_draw_rect(gamelib_t *g, int x,int y,int w,int h, gamelib_color_t c);
void gamelib_fill_rect(gamelib_t *g, int x,int y,int w,int h, gamelib_color_t c);
void gamelib_draw_circle(gamelib_t *g, int cx,int cy,int r, gamelib_color_t c);
void gamelib_fill_circle(gamelib_t *g, int cx,int cy,int r, gamelib_color_t c);
void gamelib_draw_ellipse(gamelib_t *g, int cx,int cy,int rx,int ry, gamelib_color_t c);
void gamelib_fill_ellipse(gamelib_t *g, int cx,int cy,int rx,int ry, gamelib_color_t c);
void gamelib_draw_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c);
void gamelib_fill_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c);

/* --- clip --- */
void gamelib_set_clip(gamelib_t *g, int x, int y, int w, int h);
void gamelib_clear_clip(gamelib_t *g);

/* --- text --- */
void gamelib_draw_text(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c);
void gamelib_draw_text_scale(gamelib_t *g, int x,int y, const char *s,
                             gamelib_color_t c, int scale_w, int scale_h);
void gamelib_draw_number(gamelib_t *g, int x, int y, int n, gamelib_color_t c);
void gamelib_draw_printf(gamelib_t *g, int x, int y, gamelib_color_t c, const char *fmt, ...);

/* --- sprite --- */
int  gamelib_sprite_create(gamelib_t *g, int w, int h);
int  gamelib_sprite_load_bmp(gamelib_t *g, const uint8_t *data, size_t len);
void gamelib_sprite_free(gamelib_t *g, int id);
void gamelib_sprite_set_pixel(gamelib_t *g, int id, int x, int y, gamelib_color_t c);
void gamelib_draw_sprite(gamelib_t *g, int id, int x, int y);
void gamelib_draw_sprite_region(gamelib_t *g, int id, int x,int y,
                                int sx,int sy,int sw,int sh);
void gamelib_draw_sprite_ex(gamelib_t *g, int id, int x, int y, int flags);
void gamelib_draw_sprite_scaled(gamelib_t *g, int id, int x, int y, int dst_w, int dst_h);
void gamelib_draw_sprite_frame_scaled(gamelib_t *g, int id, int x, int y,
                                       int fw, int fh, int frame, int dst_w, int dst_h,
                                       int flags);
int  gamelib_sprite_width(gamelib_t *g, int id);
int  gamelib_sprite_height(gamelib_t *g, int id);
void gamelib_sprite_set_color_key(gamelib_t *g, int id, gamelib_color_t c);

/* --- tilemap --- */
int  gamelib_tilemap_create(gamelib_t *g, int cols, int rows, int tile_size, int tileset_id);
void gamelib_tilemap_clear(gamelib_t *g, int map_id, int tile_id);
void gamelib_tilemap_set_tile(gamelib_t *g, int map_id, int col, int row, int tile_id);
void gamelib_tilemap_fill_rect(gamelib_t *g, int map_id, int col, int row, int w, int h, int tile_id);
int  gamelib_tilemap_get_cols(gamelib_t *g, int map_id);
int  gamelib_tilemap_get_rows(gamelib_t *g, int map_id);
int  gamelib_tilemap_get_tile_size(gamelib_t *g, int map_id);
void gamelib_tilemap_draw(gamelib_t *g, int map_id, int offset_x, int offset_y);
int  gamelib_tilemap_world_to_col(gamelib_t *g, int map_id, int world_x);
int  gamelib_tilemap_world_to_row(gamelib_t *g, int map_id, int world_y);
int  gamelib_tilemap_get_at_pixel(gamelib_t *g, int map_id, int world_x, int world_y);
void gamelib_tilemap_free(gamelib_t *g, int map_id);

/* --- input --- */
bool gamelib_is_key_down(gamelib_t *g, int key);
bool gamelib_is_key_pressed(gamelib_t *g, int key);
bool gamelib_is_key_released(gamelib_t *g, int key);
int  gamelib_mouse_x(gamelib_t *g);
int  gamelib_mouse_y(gamelib_t *g);

/* --- audio --- */
void gamelib_play_beep(gamelib_t *g, int freq, int duration_ms);
void gamelib_stop_beep(gamelib_t *g);

/* --- wav audio --- */
int  gamelib_play_wav(gamelib_t *g, const uint8_t *wav_data, int channel, int volume);
void gamelib_stop_wav(gamelib_t *g, int channel);
void gamelib_stop_all_wav(gamelib_t *g);
bool gamelib_is_wav_playing(gamelib_t *g, int channel);
void gamelib_set_wav_volume(gamelib_t *g, int channel, int volume);
void gamelib_set_master_volume(gamelib_t *g, int volume);
int  gamelib_get_master_volume(gamelib_t *g);

/* --- music --- */
int  gamelib_play_music(gamelib_t *g, const uint8_t *wav_data);
void gamelib_stop_music(gamelib_t *g);
bool gamelib_is_music_playing(gamelib_t *g);

/* --- font --- */
int  gamelib_font_load(gamelib_t *g, const uint8_t *ttf_data, int font_size);
void gamelib_font_free(gamelib_t *g, int font_id);
void gamelib_draw_text_font(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c, int font_id);
void gamelib_draw_printf_font(gamelib_t *g, int x, int y, gamelib_color_t c, int font_id, const char *fmt, ...);
int  gamelib_get_text_width_font(gamelib_t *g, const char *s, int font_id);

/* --- helpers --- */
int  gamelib_random(int minVal, int maxVal);
bool gamelib_rect_overlap(int x1,int y1,int w1,int h1, int x2,int y2,int w2,int h2);
bool gamelib_point_in_rect(int px,int py, int x,int y,int w,int h);
float gamelib_distance_f(int x1,int y1, int x2,int y2);

/* --- grid --- */
void gamelib_draw_grid(gamelib_t *g, int ox, int oy, int rows, int cols, int cell_size, gamelib_color_t c);
void gamelib_fill_cell(gamelib_t *g, int ox, int oy, int row, int col, int cell_size, gamelib_color_t c);

#ifdef __cplusplus
}
#endif

#endif
