/* demo_09_snake.c — Snake Game */

#include "gamelib.h"

#define SNAKE_MAX 200

static void demo_09_snake(gamelib_t *g)
{
    int gridX = 10, gridY = 28;
    int cellSize = 12;
    int rows = 20, cols = 17;

    int snakeR[SNAKE_MAX], snakeC[SNAKE_MAX];
    int len = 3;
    snakeR[0] = 10; snakeC[0] = 8;
    snakeR[1] = 10; snakeC[1] = 7;
    snakeR[2] = 10; snakeC[2] = 6;
    int dir = 3, nextDir = 3; /* 0=up 1=down 2=left 3=right */
    int foodR = 5, foodC = 12;
    int score = 0;
    bool over = false;
    double timer = 0.0;
    double interval = 0.15;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_B)) break;

        double dt = gamelib_get_delta_time(g);
        if (!over) {
            if (gamelib_is_key_down(g, KEY_UP)    && dir != 1) nextDir = 0;
            if (gamelib_is_key_down(g, KEY_DOWN)  && dir != 0) nextDir = 1;
            if (gamelib_is_key_down(g, KEY_LEFT)  && dir != 3) nextDir = 2;
            if (gamelib_is_key_down(g, KEY_RIGHT) && dir != 2) nextDir = 3;

            timer += dt;
            if (timer >= interval) {
                timer = 0; dir = nextDir;
                int nr = snakeR[0], nc = snakeC[0];
                if (dir == 0) nr--; else if (dir == 1) nr++; else if (dir == 2) nc--; else nc++;
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) { over = true; } else {
                    for (int i = 0; i < len; i++)
                        if (snakeR[i] == nr && snakeC[i] == nc) { over = true; break; }
                }
                if (!over) {
                    for (int i = len; i > 0; i--) { snakeR[i] = snakeR[i-1]; snakeC[i] = snakeC[i-1]; }
                    snakeR[0] = nr; snakeC[0] = nc;
                    if (nr == foodR && nc == foodC) {
                        score++; if (len < SNAKE_MAX) len++;
                        foodR = gamelib_random(0, rows-1); foodC = gamelib_random(0, cols-1);
                    }
                }
            }
        } else {
            if (gamelib_is_key_pressed(g, KEY_A)) {
                len = 3; snakeR[0]=10;snakeC[0]=8; snakeR[1]=10;snakeC[1]=7; snakeR[2]=10;snakeC[2]=6;
                dir = 3; nextDir = 3; score = 0; over = false;
            }
        }

        gamelib_clear(g, COLOR_BLACK);
        gamelib_draw_grid(g, gridX, gridY, rows, cols, cellSize, COLOR_DARKGRAY);
        for (int i = 0; i < len; i++)
            gamelib_fill_cell(g, gridX, gridY, snakeR[i], snakeC[i], cellSize, COLOR_GREEN);
        gamelib_fill_cell(g, gridX, gridY, foodR, foodC, cellSize, COLOR_RED);
        gamelib_draw_printf(g, 10, gridY + rows * cellSize + 4, COLOR_WHITE, "Score:%d", score);
        gamelib_draw_text(g, 5, 5, "Snake", COLOR_WHITE);
        if (over) { gamelib_draw_text(g, 60, 150, "GAME OVER", COLOR_RED); gamelib_draw_text(g, 60, 170, "A:restart", COLOR_WHITE); }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
