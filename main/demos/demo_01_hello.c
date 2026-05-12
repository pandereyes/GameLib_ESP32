/* demo_01_hello.c — Hello World
   Learn: clear, draw_text, draw_text_scale, draw_printf, get_time, is_key_pressed */

#include "gamelib.h"

static void demo_01_hello(gamelib_t *g)
{
    while (!gamelib_is_closed(g)) {
        gamelib_clear(g, COLOR_DARK_BLUE);

        gamelib_draw_text_scale(g, 10, 100, "Hello", COLOR_GOLD, 5, 5);
        gamelib_draw_text(g, 30, 150, "Welcome to GameLib!", COLOR_WHITE);
        gamelib_draw_text(g, 30, 170, "Press B to exit", COLOR_LIGHTGRAY);

        gamelib_draw_printf(g, 5, 300, COLOR_LIGHTGRAY, "Time: %.1f s", gamelib_get_time(g));

        if (gamelib_is_key_pressed(g, KEY_B)) break;

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
