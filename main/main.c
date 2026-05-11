#include <stdio.h>
#include "gamelib.h"

/* HAL registration */
void gamelib_port_register_hal(void);

static gamelib_t game;

void app_main(void)
{
    gamelib_port_register_hal();

    if (gamelib_init(&game, 240, 320, 60) != 0) {
        printf("GameLib init failed\n");
        return;
    }

    /* --- Batch1: procedural sprite (red circle) --- */
    int sprite_id = gamelib_sprite_create(&game, 16, 16);
    if (sprite_id >= 0) {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                int dx = x - 8, dy = y - 8;
                gamelib_color_t c = (dx * dx + dy * dy < 49) ? COLOR_RED : COLOR_NONE;
                gamelib_sprite_set_pixel(&game, sprite_id, x, y, c);
            }
        gamelib_sprite_set_color_key(&game, sprite_id, COLOR_NONE);
    }

    /* --- Batch2: 8x8 4-frame anim sheet --- */
    int anim_id = gamelib_sprite_create(&game, 32, 8);
    if (anim_id >= 0) {
        gamelib_color_t colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_WHITE};
        int sizes[] = {1, 2, 3, 2};
        for (int f = 0; f < 4; f++) {
            int cx = f * 8 + 4, cy = 4;
            for (int dy = -sizes[f]; dy <= sizes[f]; dy++)
                for (int dx = -sizes[f]; dx <= sizes[f]; dx++)
                    if (dx * dx + dy * dy <= sizes[f] * sizes[f])
                        gamelib_sprite_set_pixel(&game, anim_id, cx + dx, cy + dy, colors[f]);
        }
        gamelib_sprite_set_color_key(&game, anim_id, COLOR_NONE);
    }

    /* --- Batch4: Tilemap demo --- */
    /* procedural 8x8 tileset: 4 tiles */
    int tileset_id = gamelib_sprite_create(&game, 32, 8);
    if (tileset_id >= 0) {
        /* tile 0: grass (green) */
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
                gamelib_sprite_set_pixel(&game, tileset_id, x, y, COLOR_DARK_GREEN);
        /* tile 1: dirt (brown) */
        for (int y = 0; y < 8; y++)
            for (int x = 8; x < 16; x++)
                gamelib_sprite_set_pixel(&game, tileset_id, x, y, COLOR_BROWN);
        /* tile 2: brick (orange) */
        for (int y = 0; y < 8; y++)
            for (int x = 16; x < 24; x++)
                gamelib_sprite_set_pixel(&game, tileset_id, x, y, COLOR_ORANGE);
        /* tile 3: water (blue) */
        for (int y = 0; y < 8; y++)
            for (int x = 24; x < 32; x++)
                gamelib_sprite_set_pixel(&game, tileset_id, x, y, COLOR_DARK_CYAN);
    }

#define TM_COLS 6
#define TM_ROWS 5
#define TM_TS   8
    int map_id = gamelib_tilemap_create(&game, TM_COLS, TM_ROWS, TM_TS, tileset_id);
    if (map_id >= 0) {
        gamelib_tilemap_clear(&game, map_id, 0);
        /* dirt strip at bottom */
        gamelib_tilemap_fill_rect(&game, map_id, 0, TM_ROWS - 1, TM_COLS, 1, 1);
        /* brick platform */
        gamelib_tilemap_set_tile(&game, map_id, 1, 2, 2);
        gamelib_tilemap_set_tile(&game, map_id, 2, 2, 2);
        gamelib_tilemap_set_tile(&game, map_id, 3, 2, 2);
        /* water pool */
        gamelib_tilemap_set_tile(&game, map_id, 4, TM_ROWS - 1, 3);
        gamelib_tilemap_set_tile(&game, map_id, 5, TM_ROWS - 1, 3);
    }

    int player_x = 112, player_y = 152;
    int frame = 0, frame_timer = 0;
    int master_vol = 1000;
    int beep_note = 0;
    int beep_notes[] = {262, 330, 392, 523};

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_DARKGRAY);

        int w = gamelib_get_width(&game);
        int h = gamelib_get_height(&game);

        /* HUD */
        gamelib_draw_printf(&game, 5, 5, COLOR_WHITE, "GameLib ESP32 MVE v5");
        gamelib_draw_printf(&game, 5, h - 50, COLOR_GREEN, "FPS: %.0f", gamelib_get_fps(&game));
        gamelib_draw_printf(&game, 5, h - 35, COLOR_CYAN, "Time: %.1fs", gamelib_get_time(&game));
        gamelib_draw_printf(&game, 5, h - 20, COLOR_YELLOW, "Vol: %d/1000", master_vol);
        gamelib_fill_rect(&game, 80, h - 18, master_vol / 10, 8, COLOR_GOLD);

        /* Original drawing */
        gamelib_draw_rect(&game, 10, 40, 50, 30, COLOR_CYAN);
        gamelib_fill_circle(&game, 200, 55, 15, COLOR_MAGENTA);
        gamelib_draw_line(&game, 10, 80, 80, 120, COLOR_YELLOW);

        /* Color test */
        gamelib_fill_rect(&game, 10, 140, 20, 20, COLOR_RGB(255, 165, 0));
        gamelib_fill_rect(&game, 35, 140, 20, 20, COLOR_GOLD);
        gamelib_draw_text(&game, 65, 145, "Batch1 colors", COLOR_LIGHTGRAY);

        /* Sprite flip demo */
        gamelib_draw_text(&game, 10, 175, "Flip:", COLOR_WHITE);
        gamelib_draw_sprite(&game, sprite_id, 10, 190);
        gamelib_draw_sprite_ex(&game, sprite_id, 35, 190, SPRITE_FLIP_H);
        gamelib_draw_sprite_ex(&game, sprite_id, 60, 190, SPRITE_FLIP_V);
        gamelib_draw_sprite_ex(&game, sprite_id, 85, 190, SPRITE_FLIP_H | SPRITE_FLIP_V);

        /* Sprite scaled demo */
        gamelib_draw_text(&game, 10, 215, "Scale:", COLOR_WHITE);
        gamelib_draw_sprite_scaled(&game, sprite_id, 10, 230, 32, 32);
        gamelib_draw_sprite_scaled(&game, sprite_id, 50, 230, 48, 48);

        /* Frame anim demo */
        frame_timer++;
        if (frame_timer >= 10) { frame_timer = 0; frame = (frame + 1) % 4; }
        gamelib_draw_text(&game, 120, 175, "Frame Anim:", COLOR_WHITE);
        gamelib_draw_sprite_scaled(&game, anim_id, 120, 190, 64, 16);
        gamelib_draw_sprite_frame_scaled(&game, anim_id, 120, 215, 8, 8, frame, 32, 32, 0);
        gamelib_draw_sprite_frame_scaled(&game, anim_id, 160, 215, 8, 8, frame, 32, 32, SPRITE_FLIP_H);

        /* --- Batch4: Tilemap demo --- */
        int tm_x = 130, tm_y = 210;
        gamelib_draw_text(&game, tm_x, tm_y - 12, "Tilemap:", COLOR_WHITE);
        gamelib_tilemap_draw(&game, map_id, tm_x, tm_y);

        /* Input + movement */
        if (gamelib_is_key_down(&game, KEY_LEFT))  player_x -= 2;
        if (gamelib_is_key_down(&game, KEY_RIGHT)) player_x += 2;
        if (gamelib_is_key_down(&game, KEY_UP))    player_y -= 2;
        if (gamelib_is_key_down(&game, KEY_DOWN))  player_y += 2;

        if (gamelib_is_key_pressed(&game, KEY_A)) {
            beep_note = (beep_note + 1) % 4;
            gamelib_play_beep(&game, beep_notes[beep_note], 150);
        }

        if (gamelib_is_key_down(&game, KEY_B)) {
            if (gamelib_is_key_down(&game, KEY_UP) && master_vol < 1000)
                master_vol += 20;
            if (gamelib_is_key_down(&game, KEY_DOWN) && master_vol > 0)
                master_vol -= 20;
            gamelib_set_master_volume(&game, master_vol);
        }

        if (player_x < 0) player_x = 0;
        if (player_x > w - 16) player_x = w - 16;
        if (player_y < 0) player_y = 0;
        if (player_y > h - 16) player_y = h - 16;

        gamelib_draw_sprite(&game, sprite_id, player_x, player_y);

        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }

    gamelib_deinit(&game);
}
