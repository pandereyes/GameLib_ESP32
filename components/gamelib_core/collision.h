#ifndef GAMELIB_COLLISION_H
#define GAMELIB_COLLISION_H

#include <stdbool.h>
#include <stdint.h>

#define GAMELIB_COLLISION_MAX_LAYERS 4

typedef bool (*gamelib_tile_blocked_fn)(int tile_id, void *user);

typedef struct {
    const int16_t *layers[GAMELIB_COLLISION_MAX_LAYERS];
    int layer_count;
    int cols;
    int rows;
    int tile_w;
    int tile_h;
    bool out_of_bounds_blocked;
    gamelib_tile_blocked_fn is_blocked;
    void *user;
} gamelib_collision_map_t;

void gamelib_collision_map_init(gamelib_collision_map_t *map, int cols, int rows,
                                int tile_w, int tile_h,
                                gamelib_tile_blocked_fn is_blocked, void *user);
bool gamelib_collision_map_add_layer(gamelib_collision_map_t *map, const int16_t *layer);
void gamelib_collision_map_set_out_of_bounds_blocked(gamelib_collision_map_t *map, bool blocked);
bool gamelib_collision_map_is_blocked_at_tile(const gamelib_collision_map_t *map, int col, int row);
bool gamelib_collision_map_is_blocked_at_pixel(const gamelib_collision_map_t *map, int world_x, int world_y);
bool gamelib_collision_map_is_rect_blocked(const gamelib_collision_map_t *map, int x, int y, int w, int h);

#endif
