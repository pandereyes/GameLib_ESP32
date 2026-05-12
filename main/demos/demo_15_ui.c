/* demo_15_ui.c — UI Controls */

#include "gamelib.h"
#include <stdbool.h>

static void demo_15_ui(gamelib_t *g)
{
    bool musicOn = true, sfxOn = true, gridOn = false, hardMode = false;
    int difficulty = 0;
    bool paused = false, turbo = false;
    int startCount = 0, resetCount = 0;
    const char *lastEvent = "NONE";

    while (!gamelib_is_closed(g)) {
        gamelib_clear(g, COLOR_RGB(18, 22, 34));

        gamelib_fill_rect(g, 0, 0, 240, 28, COLOR_RGB(10, 14, 24));
        gamelib_draw_text(g, 4, 4, "UI CONTROLS", COLOR_WHITE);
        gamelib_draw_text(g, 4, 18, "D-Pad:nav  A:click", COLOR_LIGHTGRAY);

        gamelib_ui_begin(g);

        /* Buttons */
        gamelib_draw_text(g, 4, 36, "Buttons:", COLOR_WHITE);
        if (gamelib_button(g, 4, 50, 100, 22, "START", COLOR_RGB(52, 150, 92)))
            { startCount++; lastEvent = "START"; }
        if (gamelib_button(g, 120, 50, 100, 22, "RESET", COLOR_RGB(196, 142, 46))) {
            musicOn = sfxOn = true; gridOn = hardMode = false;
            resetCount++; lastEvent = "RESET";
        }

        /* Checkboxes */
        gamelib_draw_text(g, 4, 86, "Checkboxes:", COLOR_WHITE);
        if (gamelib_checkbox(g, 4, 100, "MUSIC", &musicOn)) lastEvent = musicOn ? "MUSIC ON" : "MUSIC OFF";
        if (gamelib_checkbox(g, 4, 118, "SFX", &sfxOn)) lastEvent = sfxOn ? "SFX ON" : "SFX OFF";
        if (gamelib_checkbox(g, 4, 136, "GRID", &gridOn)) lastEvent = gridOn ? "GRID ON" : "GRID OFF";
        if (gamelib_checkbox(g, 4, 154, "HARD MODE", &hardMode)) lastEvent = hardMode ? "HARD ON" : "HARD OFF";

        /* Radio boxes */
        gamelib_draw_text(g, 4, 180, "Difficulty:", COLOR_WHITE);
        gamelib_radio_box(g, 4, 196, "EASY", &difficulty, 0);
        gamelib_radio_box(g, 80, 196, "MEDIUM", &difficulty, 1);
        gamelib_radio_box(g, 155, 196, "HARD", &difficulty, 2);

        /* Toggle buttons */
        gamelib_draw_text(g, 4, 224, "Toggles:", COLOR_WHITE);
        gamelib_toggle_button(g, 4, 240, 100, 22, "PAUSE", &paused, COLOR_RGB(180, 76, 76));
        gamelib_toggle_button(g, 120, 240, 100, 22, "TURBO", &turbo, COLOR_RGB(52, 150, 92));

        /* Status */
        const char *diffNames[] = {"EASY", "MEDIUM", "HARD"};
        gamelib_draw_printf(g, 4, 278, COLOR_LIGHTGRAY, "Event: %s", lastEvent);
        gamelib_draw_printf(g, 4, 292, COLOR_LIGHTGRAY, "S:%d R:%d Diff:%s", startCount, resetCount, diffNames[difficulty]);
        gamelib_draw_printf(g, 4, 306, COLOR_LIGHTGRAY, "P:%s T:%s G:%s",
                           paused?"Y":"N", turbo?"Y":"N", gridOn?"Y":"N");

        gamelib_ui_end(g);
        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
