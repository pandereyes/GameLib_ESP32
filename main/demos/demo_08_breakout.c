/* demo_08_breakout.c — Breakout */

#include "gamelib.h"
#include <math.h>

static void demo_08_breakout(gamelib_t *g)
{
    int pw = 40, ph = 8;  /* paddle */
    int px = 100, py = 300;
    float bx = 120.0f, by = 280.0f;  /* ball */
    float bvx = 100.0f, bvy = -150.0f;
    int br = 5;
    bool ballActive = false;

    int bw = 30, bh = 12;  /* bricks */
    int bCols = 6, bRows = 5;
    bool bricks[6][5];
    int brickCount = 0;
    for (int c = 0; c < bCols; c++)
        for (int r = 0; r < bRows; r++)
            { bricks[c][r] = true; brickCount++; }

    int score = 0;
    bool gameOver = false;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_B)) break;

        float dt = (float)gamelib_get_delta_time(g);
        if (dt > 0.05f) dt = 0.05f;

        if (!gameOver) {
            /* paddle */
            if (gamelib_is_key_down(g, KEY_LEFT))  px -= (int)(200.0f * dt);
            if (gamelib_is_key_down(g, KEY_RIGHT)) px += (int)(200.0f * dt);
            if (px < 0) px = 0;
            if (px > 200) px = 200;

            /* launch ball */
            if (!ballActive && gamelib_is_key_pressed(g, KEY_A))
                { ballActive = true; bvx = 120.0f; bvy = -150.0f; }

            if (ballActive) {
                bx += bvx * dt; by += bvy * dt;
                if (bx - br < 0) { bx = (float)br; bvx = -bvx; }
                if (bx + br > 240) { bx = 240.0f - br; bvx = -bvx; }
                if (by - br < 0) { by = (float)br; bvy = -bvy; }
                /* paddle hit */
                if (bvy > 0 && bx >= px && bx <= px + pw && by + br >= py && by + br <= py + ph + 5)
                    { bvy = -fabsf(bvy); by = (float)(py - br); }
                /* miss */
                if (by > 320) { ballActive = false; bx = px + pw / 2.0f; by = py - 10.0f; }
                /* brick collision */
                for (int c = 0; c < bCols; c++) {
                    for (int r = 0; r < bRows; r++) {
                        if (!bricks[c][r]) continue;
                        int bkx = 15 + c * (bw + 4), bky = 30 + r * (bh + 4);
                        if (bx + br >= bkx && bx - br <= bkx + bw &&
                            by + br >= bky && by - br <= bky + bh) {
                            bricks[c][r] = false; brickCount--; score += 10;
                            float bcx = bkx + bw / 2.0f; float bcy = bky + bh / 2.0f;
                            if (fabsf(bx - bcx) / bw > fabsf(by - bcy) / bh) bvx = -bvx; else bvy = -bvy;
                        }
                    }
                }
                if (brickCount <= 0) gameOver = true;
            } else {
                bx = px + pw / 2.0f; by = py - 10.0f;
            }
        } else {
            if (gamelib_is_key_pressed(g, KEY_A)) {
                gameOver = false; score = 0; brickCount = 0;
                for (int c = 0; c < bCols; c++)
                    for (int r = 0; r < bRows; r++)
                        { bricks[c][r] = true; brickCount++; }
            }
        }

        gamelib_clear(g, COLOR_BLACK);
        gamelib_draw_rect(g, px, py, pw, ph, COLOR_CYAN);
        gamelib_fill_rect(g, px, py, pw, ph, COLOR_CYAN);
        gamelib_fill_circle(g, (int)bx, (int)by, br, COLOR_WHITE);

        gamelib_color_t brickColors[] = {COLOR_RED, COLOR_RED, COLOR_ORANGE, COLOR_ORANGE, COLOR_GREEN};
        for (int c = 0; c < bCols; c++)
            for (int r = 0; r < bRows; r++)
                if (bricks[c][r]) {
                    int bkx = 15 + c * (bw + 4), bky = 30 + r * (bh + 4);
                    gamelib_fill_rect(g, bkx, bky, bw, bh, brickColors[r]);
                }

        gamelib_draw_printf(g, 2, 2, COLOR_WHITE, "Score:%d  Bricks:%d", score, brickCount);
        if (!ballActive && !gameOver) gamelib_draw_text(g, 60, 200, "A:launch", COLOR_YELLOW);
        if (gameOver) { gamelib_draw_text_scale(g, 24, 140, "YOU WIN!", COLOR_GOLD, 3, 3); gamelib_draw_text(g, 70, 170, "A:restart", COLOR_WHITE); }
        gamelib_draw_text(g, 2, 310, "<- ->:move  A:launch", COLOR_LIGHTGRAY);

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
