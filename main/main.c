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

    /* create a procedural sprite (red circle) */
    int sprite_id = gamelib_sprite_create(&game, 16, 16);
    if (sprite_id >= 0) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                int dx = x - 8, dy = y - 8;
                gamelib_color_t c = (dx * dx + dy * dy < 49) ? COLOR_RED : COLOR_NONE;
                gamelib_sprite_set_pixel(&game, sprite_id, x, y, c);
            }
        }
        gamelib_sprite_set_color_key(&game, sprite_id, COLOR_NONE);
    }

    int player_x = 112, player_y = 152;

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_DARKGRAY);

        gamelib_draw_text(&game, 5, 5, "GameLib ESP32 MVE", COLOR_WHITE);
        gamelib_draw_text(&game, 5, 20, "Hello World!", COLOR_YELLOW);
        gamelib_draw_number(&game, 5, 300, (int)gamelib_get_fps(&game), COLOR_GREEN);
        gamelib_draw_text(&game, 40, 300, "FPS", COLOR_GREEN);

        gamelib_draw_rect(&game, 10, 40, 50, 30, COLOR_CYAN);
        gamelib_fill_circle(&game, 200, 55, 15, COLOR_MAGENTA);
        gamelib_draw_line(&game, 10, 80, 80, 120, COLOR_YELLOW);

        if (gamelib_is_key_down(&game, KEY_LEFT))  player_x -= 2;
        if (gamelib_is_key_down(&game, KEY_RIGHT)) player_x += 2;
        if (gamelib_is_key_down(&game, KEY_UP))    player_y -= 2;
        if (gamelib_is_key_down(&game, KEY_DOWN))  player_y += 2;

        if (gamelib_is_key_pressed(&game, KEY_A)) {
            gamelib_play_beep(&game, 880, 100);
        }

        if (player_x < 0) player_x = 0;
        if (player_x > 224) player_x = 224;
        if (player_y < 0) player_y = 0;
        if (player_y > 304) player_y = 304;

        gamelib_draw_sprite(&game, sprite_id, player_x, player_y);

        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }

    gamelib_deinit(&game);
}
