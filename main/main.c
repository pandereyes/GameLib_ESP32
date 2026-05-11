#include <stdio.h>
#include "gamelib.h"

void gamelib_port_register_hal(void);
static gamelib_t game;

void app_main(void)
{
    gamelib_port_register_hal();
    if (gamelib_init(&game, 240, 320, 60) != 0) {
        printf("GameLib init failed\n");
        return;
    }

    /* arrow sprite (16x16, simple triangle) */
    int arrow_id = gamelib_sprite_create(&game, 16, 16);
    if (arrow_id >= 0) {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++)
                gamelib_sprite_set_pixel(&game, arrow_id, x, y, COLOR_NONE);
        /* triangle pointing right */
        for (int y = 0; y < 16; y++) {
            int hw = y < 8 ? y + 1 : 16 - y;
            for (int x = 0; x < hw * 2; x++)
                gamelib_sprite_set_pixel(&game, arrow_id, x + 2, y, COLOR_YELLOW);
        }
        gamelib_sprite_set_color_key(&game, arrow_id, COLOR_NONE);
    }

    /* 32x8 anim sheet (4 frames x 8x8) */
    int sheet_id = gamelib_sprite_create(&game, 32, 8);
    if (sheet_id >= 0) {
        gamelib_color_t colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_WHITE};
        int sizes[] = {1, 2, 3, 2};
        for (int f = 0; f < 4; f++) {
            int cx = f * 8 + 4, cy_ = 4;
            for (int dy = -sizes[f]; dy <= sizes[f]; dy++)
                for (int dx = -sizes[f]; dx <= sizes[f]; dx++)
                    if (dx * dx + dy * dy <= sizes[f] * sizes[f])
                        gamelib_sprite_set_pixel(&game, sheet_id, cx + dx, cy_ + dy, colors[f]);
        }
        gamelib_sprite_set_color_key(&game, sheet_id, COLOR_NONE);
    }

    double angle = 0.0;
    int frame = 0, frame_timer = 0;
    bool clip_on = false;

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_DARKGRAY);

        int w = gamelib_get_width(&game);
        int h = gamelib_get_height(&game);

        gamelib_draw_printf(&game, 5, 5, COLOR_WHITE, "B6+7: Rotation & Clip");
        gamelib_draw_printf(&game, 5, 20, COLOR_LIGHTGRAY, "Angle: %.0f", angle);
        gamelib_draw_printf(&game, 5, h - 20, COLOR_GREEN, "FPS: %.0f", gamelib_get_fps(&game));
        gamelib_draw_text(&game, 5, 35, "<- -> rotate  A:clip", COLOR_LIGHTGRAY);

        /* --- left: raw sprite for comparison --- */
        gamelib_draw_text(&game, 10, 55, "Raw:", COLOR_WHITE);
        gamelib_draw_sprite(&game, arrow_id, 10, 70);
        gamelib_draw_sprite_ex(&game, arrow_id, 35, 70, SPRITE_FLIP_H);
        /* rotated at same position */
        gamelib_draw_sprite_rotated(&game, arrow_id, 80, 78, angle, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(&game, arrow_id, 110, 78, angle + 45.0, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(&game, arrow_id, 140, 78, angle + 90.0, SPRITE_COLORKEY);

        /* --- frame rotated --- */
        frame_timer++;
        if (frame_timer >= 8) { frame_timer = 0; frame = (frame + 1) % 4; }
        gamelib_draw_text(&game, 10, 240, "Frame rot:", COLOR_WHITE);
        gamelib_draw_sprite_frame_rotated(&game, sheet_id, 60, 290, 8, 8,
                                           frame, angle, 0);
        gamelib_draw_sprite_frame_rotated(&game, sheet_id, 110, 290, 8, 8,
                                           frame, angle + 45.0, 0);
        gamelib_draw_sprite_frame_rotated(&game, sheet_id, 160, 290, 8, 8,
                                           frame, angle + 90.0, 0);

        if (gamelib_is_key_down(&game, KEY_LEFT))  angle -= 2.0;
        if (gamelib_is_key_down(&game, KEY_RIGHT)) angle += 2.0;
        if (gamelib_is_key_pressed(&game, KEY_A))   clip_on = !clip_on;
        if (angle >= 360.0) angle -= 360.0;
        if (angle < 0.0)    angle += 360.0;

        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }
    gamelib_deinit(&game);
}
