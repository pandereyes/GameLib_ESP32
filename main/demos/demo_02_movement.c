/* demo_02_movement.c — Movement and Physics
   Mode A: keyboard move box. Mode B: bouncing ball with trail.
   Press START to switch. */

#include "gamelib.h"
#include <stdbool.h>

static void demo_02_movement(gamelib_t *g)
{
    float boxX = 100.0f, boxY = 140.0f;
    int   boxSize = 14;
    float boxSpeed = 120.0f;

    float ballX = 120.0f, ballY = 160.0f;
    float ballVX = 100.0f, ballVY = 80.0f;
    int   ballR = 12;

    float trailX[32], trailY[32];
    int   trailCount = 0;
    bool  showBall = false;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_B)) break;
        if (gamelib_is_key_pressed(g, KEY_START)) showBall = !showBall;

        float dt = (float)gamelib_get_delta_time(g);
        if (dt > 0.05f) dt = 0.05f;

        if (!showBall) {
            if (gamelib_is_key_down(g, KEY_LEFT))  boxX -= boxSpeed * dt;
            if (gamelib_is_key_down(g, KEY_RIGHT)) boxX += boxSpeed * dt;
            if (gamelib_is_key_down(g, KEY_UP))    boxY -= boxSpeed * dt;
            if (gamelib_is_key_down(g, KEY_DOWN))  boxY += boxSpeed * dt;

            int w = gamelib_get_width(g), h = gamelib_get_height(g);
            if (boxX < 0) boxX = 0;
            if (boxY < 0) boxY = 0;
            if (boxX + boxSize > w)  boxX = (float)(w - boxSize);
            if (boxY + boxSize > h) boxY = (float)(h - boxSize);
        } else {
            ballX += ballVX * dt;
            ballY += ballVY * dt;

            int w = gamelib_get_width(g), h = gamelib_get_height(g);
            if (ballX - ballR < 0)  { ballX = (float)ballR; ballVX = -ballVX; }
            if (ballX + ballR > w)  { ballX = (float)(w - ballR); ballVX = -ballVX; }
            if (ballY - ballR < 0)  { ballY = (float)ballR; ballVY = -ballVY; }
            if (ballY + ballR > h) { ballY = (float)(h - ballR); ballVY = -ballVY; }

            if (trailCount < 32) {
                trailX[trailCount] = ballX;
                trailY[trailCount] = ballY;
                trailCount++;
            } else {
                for (int i = 0; i < 31; i++) {
                    trailX[i] = trailX[i+1];
                    trailY[i] = trailY[i+1];
                }
                trailX[31] = ballX;
                trailY[31] = ballY;
            }
        }

        gamelib_clear(g, COLOR_BLACK);

        if (!showBall) {
            gamelib_fill_rect(g, (int)boxX, (int)boxY, boxSize, boxSize, COLOR_CYAN);
            gamelib_draw_text(g, 5, 5, "Mode A: Arrow keys move box", COLOR_WHITE);
            gamelib_draw_text(g, 5, 300, "START: switch  B: quit", COLOR_LIGHTGRAY);
        } else {
            for (int i = 0; i < trailCount; i++) {
                int brightness = 40 + i * 6;
                if (brightness > 255) brightness = 255;
                int tr = 2 + i * ballR / 32;
                gamelib_fill_circle(g, (int)trailX[i], (int)trailY[i], tr,
                                    COLOR_RGB(brightness, 0, 0));
            }
            gamelib_fill_circle(g, (int)ballX, (int)ballY, ballR, COLOR_RED);
            gamelib_draw_circle(g, (int)ballX, (int)ballY, ballR, COLOR_WHITE);
            gamelib_draw_text(g, 5, 5, "Mode B: Bouncing Ball", COLOR_WHITE);
        }

        gamelib_draw_printf(g, 120, 300, COLOR_LIGHTGRAY,
                            "FPS: %.0f", gamelib_get_fps(g));
        gamelib_update(g);
        gamelib_wait_frame(g);
    }
}
