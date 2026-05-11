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

    bool music_on = true, sfx_on = true, grid_on = false, hard_mode = false;
    int  difficulty = 0;
    bool paused = false, turbo = false;
    int  start_n = 0, reset_n = 0;

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_RGB(18, 22, 34));

        gamelib_fill_rect(&game, 0, 0, 240, 40, COLOR_RGB(10, 14, 24));
        gamelib_draw_text(&game, 8, 8, "UI CONTROLS", COLOR_WHITE);
        gamelib_draw_text(&game, 8, 24, "D-Pad:nav  A:click", COLOR_LIGHTGRAY);

        gamelib_ui_begin(&game);

        gamelib_draw_text(&game, 8, 50, "Buttons:", COLOR_WHITE);
        if (gamelib_button(&game, 8, 68, 100, 24, "START", COLOR_RGB(52, 150, 92)))
            start_n++;
        if (gamelib_button(&game, 120, 68, 100, 24, "RESET", COLOR_RGB(196, 142, 46))) {
            music_on = true; sfx_on = true; grid_on = false; hard_mode = false;
            reset_n++;
        }

        gamelib_draw_text(&game, 8, 108, "Checkboxes:", COLOR_WHITE);
        gamelib_checkbox(&game, 8, 128, "MUSIC", &music_on);
        gamelib_checkbox(&game, 8, 148, "SFX", &sfx_on);
        gamelib_checkbox(&game, 8, 168, "GRID", &grid_on);
        gamelib_checkbox(&game, 8, 188, "HARD MODE", &hard_mode);

        gamelib_draw_text(&game, 8, 220, "Difficulty:", COLOR_WHITE);
        gamelib_radio_box(&game, 8, 238, "EASY", &difficulty, 0);
        gamelib_radio_box(&game, 80, 238, "MEDIUM", &difficulty, 1);
        gamelib_radio_box(&game, 160, 238, "HARD", &difficulty, 2);

        gamelib_draw_text(&game, 8, 268, "Toggles:", COLOR_WHITE);
        gamelib_toggle_button(&game, 8, 286, 100, 24, "PAUSE", &paused, COLOR_RGB(180, 76, 76));
        gamelib_toggle_button(&game, 120, 286, 100, 24, "TURBO", &turbo, COLOR_RGB(52, 150, 92));

        const char *diff_names[] = {"EASY", "MEDIUM", "HARD"};
        gamelib_draw_printf(&game, 5, 310, COLOR_LIGHTGRAY, "S:%d R:%d DIFF:%s",
                           start_n, reset_n, diff_names[difficulty]);
        gamelib_draw_printf(&game, 5, 318, COLOR_LIGHTGRAY, "PAUSE:%s TURBO:%s",
                           paused ? "Y" : "N", turbo ? "Y" : "N");

        gamelib_ui_end(&game);
        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }
    gamelib_deinit(&game);
}
