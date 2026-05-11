#ifndef TILEMAP_H
#define TILEMAP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t *tiles;
    int      cols;
    int      rows;
    int      tile_size;
    int      tileset_id;
    bool     used;
} tilemap_t;

#endif
