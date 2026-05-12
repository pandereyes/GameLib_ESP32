/* demo_10_tilemap.c — Tilemap Scrolling Demo */

#include "gamelib.h"

#define TM10_TS 16
#define TM10_COLS 40
#define TM10_ROWS 40

enum { T10_SKY, T10_GRASS, T10_DIRT, T10_STONE, T10_WATER, T10_BRICK, T10_WOOD, T10_LEAVES, T10_SAND, T10_CLOUD, T10_MAX_TILES };

static int demo_10_make_tileset(gamelib_t *g)
{
    int id = gamelib_sprite_create(g, T10_MAX_TILES * TM10_TS, TM10_TS);
    if (id < 0) return -1;
    
    for (int y = 0; y < TM10_TS; y++) {
        for (int x = 0; x < TM10_TS; x++) {
            /* sky */
            gamelib_sprite_set_pixel(g, id, T10_SKY * TM10_TS + x, y, COLOR_CYAN);
            /* grass */
            gamelib_sprite_set_pixel(g, id, T10_GRASS * TM10_TS + x, y, COLOR_DARK_GREEN);
            if (x % 4 == 0 && y % 4 == 0) gamelib_sprite_set_pixel(g, id, T10_GRASS * TM10_TS + x, y, COLOR_GREEN);
            /* dirt */
            gamelib_sprite_set_pixel(g, id, T10_DIRT * TM10_TS + x, y, COLOR_BROWN);
            if ((x + y) % 5 == 0) gamelib_sprite_set_pixel(g, id, T10_DIRT * TM10_TS + x, y, COLOR_DARKGRAY);
            /* stone */
            gamelib_sprite_set_pixel(g, id, T10_STONE * TM10_TS + x, y, COLOR_GRAY);
            if (x == 0 || y == 0) gamelib_sprite_set_pixel(g, id, T10_STONE * TM10_TS + x, y, COLOR_LIGHTGRAY);
            /* water */
            gamelib_sprite_set_pixel(g, id, T10_WATER * TM10_TS + x, y, COLOR_BLUE);
            if ((x + y + (int)(gamelib_get_time(g) / 100)) % 4 == 0) gamelib_sprite_set_pixel(g, id, T10_WATER * TM10_TS + x, y, COLOR_CYAN);
            /* brick */
            gamelib_sprite_set_pixel(g, id, T10_BRICK * TM10_TS + x, y, COLOR_RED);
            if (x % 8 == 0 || y % 4 == 0) gamelib_sprite_set_pixel(g, id, T10_BRICK * TM10_TS + x, y, COLOR_DARKGRAY);
            /* wood */
            gamelib_sprite_set_pixel(g, id, T10_WOOD * TM10_TS + x, y, COLOR_ORANGE);
            if (x % 4 == 1) gamelib_sprite_set_pixel(g, id, T10_WOOD * TM10_TS + x, y, COLOR_BROWN);
            /* leaves */
            gamelib_sprite_set_pixel(g, id, T10_LEAVES * TM10_TS + x, y, COLOR_GREEN);
            if ((x * y) % 3 == 0) gamelib_sprite_set_pixel(g, id, T10_LEAVES * TM10_TS + x, y, COLOR_DARK_GREEN);
            /* sand */
            gamelib_sprite_set_pixel(g, id, T10_SAND * TM10_TS + x, y, COLOR_YELLOW);
            if ((x * y) % 7 == 0) gamelib_sprite_set_pixel(g, id, T10_SAND * TM10_TS + x, y, COLOR_ORANGE);
            /* cloud */
            gamelib_sprite_set_pixel(g, id, T10_CLOUD * TM10_TS + x, y, COLOR_WHITE);
            if (y > 10) gamelib_sprite_set_pixel(g, id, T10_CLOUD * TM10_TS + x, y, COLOR_LIGHTGRAY);
        }
    }
    return id;
}

static void demo_10_tilemap(gamelib_t *g)
{
    int ts_id = demo_10_make_tileset(g);
    int map_id = gamelib_tilemap_create(g, TM10_COLS, TM10_ROWS, TM10_TS, ts_id);
    if (map_id < 0) return;

    /* build a more detailed map */
    gamelib_tilemap_clear(g, map_id, T10_SKY);
    
    /* clouds in the sky */
    gamelib_tilemap_fill_rect(g, map_id, 3, 3, 4, 2, T10_CLOUD);
    gamelib_tilemap_fill_rect(g, map_id, 15, 6, 5, 3, T10_CLOUD);
    gamelib_tilemap_fill_rect(g, map_id, 28, 4, 6, 2, T10_CLOUD);
    
    /* ground terrain */
    gamelib_tilemap_fill_rect(g, map_id, 0, TM10_ROWS / 2, TM10_COLS, TM10_ROWS / 2, T10_DIRT);
    gamelib_tilemap_fill_rect(g, map_id, 0, TM10_ROWS / 2, TM10_COLS, 1, T10_GRASS);
    
    /* underground stone structures */
    gamelib_tilemap_fill_rect(g, map_id, 5, TM10_ROWS / 2 + 5, 8, 4, T10_STONE);
    gamelib_tilemap_fill_rect(g, map_id, 20, TM10_ROWS / 2 + 3, 10, 8, T10_STONE);
    
    /* a lake */
    gamelib_tilemap_fill_rect(g, map_id, 10, TM10_ROWS / 2 + 1, 8, 3, T10_WATER);
    gamelib_tilemap_fill_rect(g, map_id, 9, TM10_ROWS / 2, 10, 1, T10_SAND); // sandy shore
    gamelib_tilemap_fill_rect(g, map_id, 10, TM10_ROWS / 2, 8, 1, T10_WATER);

    /* a house */
    int hx = 25, hy = TM10_ROWS / 2 - 4;
    gamelib_tilemap_fill_rect(g, map_id, hx, hy, 5, 4, T10_BRICK);
    gamelib_tilemap_fill_rect(g, map_id, hx - 1, hy - 1, 7, 1, T10_WOOD); // roof base
    gamelib_tilemap_fill_rect(g, map_id, hx, hy - 2, 5, 1, T10_WOOD);
    gamelib_tilemap_fill_rect(g, map_id, hx + 1, hy - 3, 3, 1, T10_WOOD);
    gamelib_tilemap_set_tile(g, map_id, hx + 2, hy + 2, T10_WOOD); // door
    gamelib_tilemap_set_tile(g, map_id, hx + 2, hy + 3, T10_WOOD);

    /* some trees */
    for (int tx = 3; tx < TM10_COLS; tx += 9) {
        if (tx >= hx - 2 && tx <= hx + 6) continue; // avoid house
        if (tx >= 9 && tx <= 19) continue; // avoid lake
        int ty = TM10_ROWS / 2;
        gamelib_tilemap_fill_rect(g, map_id, tx, ty - 3, 1, 3, T10_WOOD); // trunk
        gamelib_tilemap_fill_rect(g, map_id, tx - 1, ty - 5, 3, 3, T10_LEAVES); // leaves
        gamelib_tilemap_set_tile(g, map_id, tx, ty - 6, T10_LEAVES);
    }


    int cameraX = 0, cameraY = 0;
    int worldW = TM10_COLS * TM10_TS;
    int worldH = TM10_ROWS * TM10_TS;

    /* player marker */
    int px = worldW / 2, py = worldH - 60;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_B)) break;

        int sw = gamelib_get_width(g), sh = gamelib_get_height(g);

        if (gamelib_is_key_down(g, KEY_LEFT))  { px -= 3; if (px < 0) px = 0; }
        if (gamelib_is_key_down(g, KEY_RIGHT)) { px += 3; if (px > worldW - 8) px = worldW - 8; }
        if (gamelib_is_key_down(g, KEY_UP))    { py -= 3; if (py < 0) py = 0; }
        if (gamelib_is_key_down(g, KEY_DOWN))  { py += 3; if (py > worldH - 8) py = worldH - 8; }

        cameraX = px - sw / 2;
        cameraY = py - sh / 2;
        if (cameraX < 0) cameraX = 0;
        if (cameraY < 0) cameraY = 0;
        if (cameraX > worldW - sw) cameraX = worldW - sw;
        if (cameraY > worldH - sh) cameraY = worldH - sh;

        gamelib_clear(g, COLOR_SKY_BLUE);
        gamelib_tilemap_draw(g, map_id, -cameraX, -cameraY);

        /* player marker on screen */
        gamelib_fill_rect(g, px - cameraX - 3, py - cameraY - 3, 7, 7, COLOR_RED);

        /* HUD */
        int tc = gamelib_tilemap_world_to_col(g, map_id, px);
        int tr = gamelib_tilemap_world_to_row(g, map_id, py);
        gamelib_draw_printf(g, 2, 2, COLOR_WHITE, "Tilemap  Cam:%d,%d  Tile:%d,%d",
                           cameraX, cameraY, tc, tr);
        gamelib_draw_text(g, 2, 310, "Arrows:move player  B:quit", COLOR_LIGHTGRAY);

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
    gamelib_tilemap_free(g, map_id);
    gamelib_sprite_free(g, ts_id);
}
