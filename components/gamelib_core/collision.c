#include "collision.h"
#include <string.h>

static int floor_div_int(int value, int divisor)
{
    if (divisor <= 0) return 0;
    if (value >= 0) return value / divisor;
    return -(((-value) + divisor - 1) / divisor);
}

void gamelib_collision_map_init(gamelib_collision_map_t *map, int cols, int rows,
                                int tile_w, int tile_h,
                                gamelib_tile_blocked_fn is_blocked, void *user)
{
    if (!map) return;
    memset(map, 0, sizeof(*map));
    map->cols = cols;
    map->rows = rows;
    map->tile_w = tile_w;
    map->tile_h = tile_h;
    map->out_of_bounds_blocked = true;
    map->is_blocked = is_blocked;
    map->user = user;
}

bool gamelib_collision_map_add_layer(gamelib_collision_map_t *map, const int16_t *layer)
{
    if (!map || !layer || map->layer_count >= GAMELIB_COLLISION_MAX_LAYERS) {
        return false;
    }
    map->layers[map->layer_count++] = layer;
    return true;
}

void gamelib_collision_map_set_out_of_bounds_blocked(gamelib_collision_map_t *map, bool blocked)
{
    if (!map) return;
    map->out_of_bounds_blocked = blocked;
}

bool gamelib_collision_map_is_blocked_at_tile(const gamelib_collision_map_t *map, int col, int row)
{
    if (!map || map->cols <= 0 || map->rows <= 0 || !map->is_blocked) {
        return false;
    }
    if (col < 0 || col >= map->cols || row < 0 || row >= map->rows) {
        return map->out_of_bounds_blocked;
    }

    int idx = row * map->cols + col;
    for (int i = 0; i < map->layer_count; i++) {
        int tile_id = map->layers[i][idx];
        if (map->is_blocked(tile_id, map->user)) {
            return true;
        }
    }
    return false;
}

bool gamelib_collision_map_is_blocked_at_pixel(const gamelib_collision_map_t *map, int world_x, int world_y)
{
    if (!map || map->tile_w <= 0 || map->tile_h <= 0) return false;
    int col = floor_div_int(world_x, map->tile_w);
    int row = floor_div_int(world_y, map->tile_h);
    return gamelib_collision_map_is_blocked_at_tile(map, col, row);
}

bool gamelib_collision_map_is_rect_blocked(const gamelib_collision_map_t *map, int x, int y, int w, int h)
{
    if (!map || w <= 0 || h <= 0 || map->tile_w <= 0 || map->tile_h <= 0) {
        return false;
    }

    int col0 = floor_div_int(x, map->tile_w);
    int row0 = floor_div_int(y, map->tile_h);
    int col1 = floor_div_int(x + w - 1, map->tile_w);
    int row1 = floor_div_int(y + h - 1, map->tile_h);

    for (int row = row0; row <= row1; row++) {
        for (int col = col0; col <= col1; col++) {
            if (gamelib_collision_map_is_blocked_at_tile(map, col, row)) {
                return true;
            }
        }
    }
    return false;
}
