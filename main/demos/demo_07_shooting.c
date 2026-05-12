/* demo_07_shooting.c — Shooting Game */

#include "gamelib.h"

#define DEMO07_MAX_ENEMIES 12
#define DEMO07_MAX_BULLETS 10

static void demo_07_shooting(gamelib_t *g)
{
    int pw = 12, ph = 12;
    int px = 110, py = 260;
    float speed = 200.0f;

    typedef struct { int x, y; bool alive; } actor_t;
    actor_t enemies[DEMO07_MAX_ENEMIES];
    actor_t bullets[DEMO07_MAX_BULLETS];
    int enemyTimer = 0;
    int score = 0;
    bool gameOver = false;

    for (int i = 0; i < DEMO07_MAX_ENEMIES; i++) enemies[i].alive = false;
    for (int i = 0; i < DEMO07_MAX_BULLETS; i++) bullets[i].alive = false;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_B)) break;

        float dt = (float)gamelib_get_delta_time(g);
        if (dt > 0.05f) dt = 0.05f;

        if (!gameOver) {
            /* Player movement */
            if (gamelib_is_key_down(g, KEY_LEFT))  px -= (int)(speed * dt);
            if (gamelib_is_key_down(g, KEY_RIGHT)) px += (int)(speed * dt);
            if (gamelib_is_key_down(g, KEY_UP))    py -= (int)(speed * dt);
            if (gamelib_is_key_down(g, KEY_DOWN))  py += (int)(speed * dt);
            if (px < 0) px = 0;
            if (px > 228) px = 228;
            if (py < 160) py = 160;
            if (py > 308) py = 308;

            /* Shoot */
            if (gamelib_is_key_pressed(g, KEY_A)) {
                for (int i = 0; i < DEMO07_MAX_BULLETS; i++) {
                    if (!bullets[i].alive) { bullets[i].x = px + 4; bullets[i].y = py; bullets[i].alive = true; break; }
                }
            }

            /* Enemy spawn */
            enemyTimer++; if (enemyTimer >= 50) { enemyTimer = 0;
                for (int i = 0; i < DEMO07_MAX_ENEMIES; i++) {
                    if (!enemies[i].alive) {
                        enemies[i].x = gamelib_random(0, 226); enemies[i].y = 0; enemies[i].alive = true; break;
                    }
                }
            }

            /* Update bullets */
            for (int i = 0; i < DEMO07_MAX_BULLETS; i++) {
                if (bullets[i].alive) { bullets[i].y -= 6; if (bullets[i].y < 0) bullets[i].alive = false; }
            }
            /* Update enemies */
            for (int i = 0; i < DEMO07_MAX_ENEMIES; i++) {
                if (enemies[i].alive) { enemies[i].y += 2; if (enemies[i].y > 320) { gameOver = true; break; } }
            }
            /* Collision */
            for (int b = 0; b < DEMO07_MAX_BULLETS; b++) {
                if (!bullets[b].alive) continue;
                for (int e = 0; e < DEMO07_MAX_ENEMIES; e++) {
                    if (!enemies[e].alive) continue;
                    if (bullets[b].x >= enemies[e].x - 6 && bullets[b].x <= enemies[e].x + 12 &&
                        bullets[b].y >= enemies[e].y - 6 && bullets[b].y <= enemies[e].y + 12) {
                        enemies[e].alive = false; bullets[b].alive = false; score++;
                    }
                }
            }
        } else {
            if (gamelib_is_key_pressed(g, KEY_A)) {
                gameOver = false; score = 0; px = 110; py = 260;
                for (int i = 0; i < DEMO07_MAX_ENEMIES; i++) enemies[i].alive = false;
                for (int i = 0; i < DEMO07_MAX_BULLETS; i++) bullets[i].alive = false;
            }
        }

        /* Draw */
        gamelib_clear(g, COLOR_BLACK);
        if (gameOver) {
            gamelib_draw_text_scale(g, 0, 130, "GAME OVER", COLOR_RED, 3, 3);
            gamelib_draw_text(g, 60, 160, "A:restart  B:quit", COLOR_WHITE);
        } else {
            /* Player */
            gamelib_fill_rect(g, px, py, pw, ph, COLOR_CYAN);
            /* Bullets */
            for (int i = 0; i < DEMO07_MAX_BULLETS; i++)
                if (bullets[i].alive) gamelib_fill_rect(g, bullets[i].x, bullets[i].y, 4, 8, COLOR_YELLOW);
            /* Enemies */
            for (int i = 0; i < DEMO07_MAX_ENEMIES; i++)
                if (enemies[i].alive) gamelib_fill_rect(g, enemies[i].x, enemies[i].y, 14, 14, COLOR_RED);
            /* Head-up display */
            gamelib_draw_printf(g, 2, 2, COLOR_WHITE, "Score:%d", score);
            gamelib_draw_text(g, 2, 310, "Arrows:move  A:fire", COLOR_LIGHTGRAY);
        }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
