/* demo_03_shapes.c — Shape Gallery
   Line fan, filled rects, circles, ellipses, triangles, combined scene. */

#include "gamelib.h"
#include <stdbool.h>
#include <math.h>

static void demo_03_draw_checker(gamelib_t *g, int x, int y, int w, int h, int cell)
{
    for (int py = 0; py < h; py += cell) {
        for (int px = 0; px < w; px += cell) {
            bool dark = ((px / cell + py / cell) & 1) != 0;
            gamelib_color_t color = dark ? COLOR_RGB(55, 65, 80) : COLOR_RGB(85, 95, 110);
            int cw = cell, ch = cell;
            if (px + cw > w) cw = w - px;
            if (py + ch > h) ch = h - py;
            gamelib_fill_rect(g, x + px, y + py, cw, ch, color);
        }
    }
}

static void demo_03_shapes(gamelib_t *g)
{
    while (!gamelib_is_closed(g)) {
        double t = gamelib_get_time(g);
        int swing = (int)(sin(t * 2.0) * 8.0);

        gamelib_clear(g, COLOR_RGB(24, 28, 36));
        gamelib_draw_text(g, 2, 2, "03 Shapes", COLOR_WHITE);

        /* Panel 1: Pixels + Lines */
        gamelib_draw_text(g, 4, 14, "Lines", COLOR_WHITE);
        gamelib_fill_rect(g, 2, 24, 116, 56, COLOR_RGB(15, 18, 24));
        for (int i = 0; i < 8; i++) {
            int sy = 28 + i * 6;
            gamelib_draw_line(g, 6, 52 + swing / 2, 114, sy,
                              COLOR_RGB(255 - i * 25, 80 + i * 15, 220));
        }

        /* Panel 2: Rectangles */
        gamelib_draw_text(g, 124, 14, "Rects", COLOR_WHITE);
        demo_03_draw_checker(g, 122, 24, 116, 56, 8);
        gamelib_draw_rect(g, 132, 30, 40, 26, COLOR_RED);
        gamelib_fill_rect(g, 192, 34, 30, 30, COLOR_GREEN);
        gamelib_fill_rect(g, 202, 44, 30, 30, COLOR_YELLOW);

        /* Panel 3: Circles + Ellipses */
        gamelib_draw_text(g, 4, 88, "Circles", COLOR_WHITE);
        demo_03_draw_checker(g, 2, 98, 116, 56, 8);
        gamelib_draw_circle(g, 30, 118, 18, COLOR_CYAN);
        gamelib_fill_circle(g, 60, 128, 14, COLOR_MAGENTA);
        gamelib_draw_ellipse(g, 96, 118, 18, 12, COLOR_GOLD);

        /* Panel 4: Triangles */
        gamelib_draw_text(g, 124, 88, "Triangles", COLOR_WHITE);
        demo_03_draw_checker(g, 122, 98, 116, 56, 8);
        gamelib_draw_triangle(g, 136, 148, 126, 110, 160, 122, COLOR_GOLD);
        gamelib_fill_triangle(g, 180, 140, 160, 108, 216, 130, COLOR_CYAN);

        /* Panel 5: Combined scene */
        gamelib_draw_text(g, 4, 162, "Scene", COLOR_WHITE);
        gamelib_fill_rect(g, 2, 172, 236, 130, COLOR_RGB(30, 44, 72));
        gamelib_fill_ellipse(g, 120, 200, 50, 20, COLOR_RGB(120, 200, 100));
        gamelib_fill_circle(g, 200, 200, 16, COLOR_RGB(255, 230, 80));
        gamelib_fill_rect(g, 80, 240, 70, 50, COLOR_BROWN);
        gamelib_fill_triangle(g, 70, 240, 115, 200, 160, 240, COLOR_DARK_RED);
        gamelib_fill_rect(g, 98, 260, 16, 30, COLOR_DARK_BLUE);
        gamelib_fill_rect(g, 80, 248, 12, 12, COLOR_RGB(160, 230, 255));
        gamelib_fill_rect(g, 138, 248, 12, 12, COLOR_RGB(160, 230, 255));

        if (gamelib_is_key_pressed(g, KEY_B)) break;
        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
