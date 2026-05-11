#include "tilemap.h"
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

#define MAX_TILEMAPS 8
extern tilemap_t *g_tilemaps;

static tilemap_t* tilemap_get(int map_id)
{
    if (map_id < 0 || map_id >= MAX_TILEMAPS) return NULL;
    if (!g_tilemaps || !g_tilemaps[map_id].used) return NULL;
    return &g_tilemaps[map_id];
}

int gamelib_tilemap_create(gamelib_t *g, int cols, int rows, int tile_size, int tileset_id)
{
    (void)g;
    if (!g_tilemaps) return -1;
    for (int i = 0; i < MAX_TILEMAPS; i++) {
        if (!g_tilemaps[i].used) {
            tilemap_t *tm = &g_tilemaps[i];
            tm->tiles = (int16_t*)calloc(cols * rows, sizeof(int16_t));
            if (!tm->tiles) return -1;
            for (int j = 0; j < cols * rows; j++) tm->tiles[j] = -1;
            tm->cols = cols;
            tm->rows = rows;
            tm->tile_size = tile_size;
            tm->tileset_id = tileset_id;
            tm->used = true;
            return i;
        }
    }
    return -1;
}

void gamelib_tilemap_free(gamelib_t *g, int map_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    free(tm->tiles);
    memset(tm, 0, sizeof(*tm));
}

void gamelib_tilemap_clear(gamelib_t *g, int map_id, int tile_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    int n = tm->cols * tm->rows;
    for (int i = 0; i < n; i++) tm->tiles[i] = (int16_t)tile_id;
}

void gamelib_tilemap_set_tile(gamelib_t *g, int map_id, int col, int row, int tile_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm || col < 0 || col >= tm->cols || row < 0 || row >= tm->rows) return;
    tm->tiles[row * tm->cols + col] = (int16_t)tile_id;
}

void gamelib_tilemap_fill_rect(gamelib_t *g, int map_id, int col, int row, int w, int h, int tile_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    for (int r = row; r < row + h && r < tm->rows; r++)
        for (int c = col; c < col + w && c < tm->cols; c++)
            if (r >= 0 && c >= 0) tm->tiles[r * tm->cols + c] = (int16_t)tile_id;
}

int gamelib_tilemap_get_cols(gamelib_t *g, int map_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    return tm ? tm->cols : 0;
}

int gamelib_tilemap_get_rows(gamelib_t *g, int map_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    return tm ? tm->rows : 0;
}

int gamelib_tilemap_get_tile_size(gamelib_t *g, int map_id)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    return tm ? tm->tile_size : 0;
}

void gamelib_tilemap_draw(gamelib_t *g, int map_id, int offset_x, int offset_y)
{
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    int ts = tm->tile_size;
    int screen_w = g->fb.width;
    int screen_h = g->fb.height;

    int start_col = (-offset_x) / ts;
    if (start_col < 0) start_col = 0;
    int start_row = (-offset_y) / ts;
    if (start_row < 0) start_row = 0;
    int end_col = (screen_w - offset_x + ts - 1) / ts;
    if (end_col > tm->cols) end_col = tm->cols;
    int end_row = (screen_h - offset_y + ts - 1) / ts;
    if (end_row > tm->rows) end_row = tm->rows;

    for (int row = start_row; row < end_row; row++) {
        for (int col = start_col; col < end_col; col++) {
            int tid = tm->tiles[row * tm->cols + col];
            if (tid < 0) continue;
            int sx = tid * ts;
            int sy = 0;
            if (tm->tileset_id >= 0) {
                int sp_w = gamelib_sprite_width(g, tm->tileset_id);
                int cols_in_sheet = sp_w / ts;
                if (cols_in_sheet > 0) {
                    sy = (tid / cols_in_sheet) * ts;
                    sx = (tid % cols_in_sheet) * ts;
                }
            }
            int dx = col * ts + offset_x;
            int dy = row * ts + offset_y;
            gamelib_draw_sprite_region(g, tm->tileset_id, dx, dy, sx, sy, ts, ts);
        }
    }
}

int gamelib_tilemap_world_to_col(gamelib_t *g, int map_id, int world_x)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    return tm ? world_x / tm->tile_size : 0;
}

int gamelib_tilemap_world_to_row(gamelib_t *g, int map_id, int world_y)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    return tm ? world_y / tm->tile_size : 0;
}

int gamelib_tilemap_get_at_pixel(gamelib_t *g, int map_id, int world_x, int world_y)
{
    (void)g;
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return -1;
    int c = world_x / tm->tile_size;
    int r = world_y / tm->tile_size;
    if (c < 0 || c >= tm->cols || r < 0 || r >= tm->rows) return -1;
    return tm->tiles[r * tm->cols + c];
}
