#ifndef GAMELIB_ANIM_H
#define GAMELIB_ANIM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GAMELIB_ANIM_LOOP,
    GAMELIB_ANIM_ONCE,
    GAMELIB_ANIM_HOLD_LAST
} gamelib_anim_mode_t;

typedef struct {
    int atlas_x;
    int atlas_y;
    int frame_w;
    int frame_h;
    int frame_count;
    uint32_t frame_ms;
    gamelib_anim_mode_t mode;
} gamelib_anim_clip_t;

typedef struct {
    const gamelib_anim_clip_t *clip;
    int frame;
    uint32_t elapsed_ms;
    uint32_t last_time_ms;
    bool has_time_sample;
    bool finished;
    bool frame_changed;
} gamelib_animator_t;

void gamelib_animator_init(gamelib_animator_t *a);
void gamelib_animator_play(gamelib_animator_t *a, const gamelib_anim_clip_t *clip);
void gamelib_animator_set_clip(gamelib_animator_t *a, const gamelib_anim_clip_t *clip);
void gamelib_animator_update_ms(gamelib_animator_t *a, uint32_t delta_ms);
int  gamelib_animator_frame(const gamelib_animator_t *a);
bool gamelib_animator_finished(const gamelib_animator_t *a);
bool gamelib_animator_frame_changed(const gamelib_animator_t *a);
const gamelib_anim_clip_t *gamelib_animator_clip(const gamelib_animator_t *a);

#endif
