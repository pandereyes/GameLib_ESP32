#include "gamelib.h"

void gamelib_play_beep(gamelib_t *g, int freq, int duration_ms)
{
    (void)g;
    if (g_hal.audio.beep) {
        g_hal.audio.beep(freq, duration_ms);
    }
}

void gamelib_stop_beep(gamelib_t *g)
{
    (void)g;
    if (g_hal.audio.stop) {
        g_hal.audio.stop();
    }
}
