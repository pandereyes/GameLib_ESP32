#include "anim.h"
#include <string.h>

static bool anim_clip_valid(const gamelib_anim_clip_t *clip)
{
    return clip && clip->frame_count > 0 && clip->frame_ms > 0;
}

void gamelib_animator_init(gamelib_animator_t *a)
{
    if (!a) return;
    memset(a, 0, sizeof(*a));
}

void gamelib_animator_play(gamelib_animator_t *a, const gamelib_anim_clip_t *clip)
{
    if (!a) return;
    a->clip = clip;
    a->frame = 0;
    a->elapsed_ms = 0;
    a->last_time_ms = 0;
    a->has_time_sample = false;
    a->finished = !anim_clip_valid(clip);
    a->frame_changed = true;
}

void gamelib_animator_set_clip(gamelib_animator_t *a, const gamelib_anim_clip_t *clip)
{
    if (!a || a->clip == clip) return;
    a->clip = clip;
    if (!anim_clip_valid(clip)) {
        a->frame = 0;
        a->finished = true;
    } else {
        if (a->frame >= clip->frame_count) {
            a->frame = clip->frame_count - 1;
        }
        a->finished = false;
    }
    a->elapsed_ms = 0;
    a->last_time_ms = 0;
    a->has_time_sample = false;
    a->frame_changed = false;
}

void gamelib_animator_update_ms(gamelib_animator_t *a, uint32_t delta_ms)
{
    if (!a) return;
    a->frame_changed = false;

    const gamelib_anim_clip_t *clip = a->clip;
    if (!anim_clip_valid(clip) || a->finished) return;

    a->elapsed_ms += delta_ms;
    while (a->elapsed_ms >= clip->frame_ms && !a->finished) {
        a->elapsed_ms -= clip->frame_ms;

        if (a->frame + 1 < clip->frame_count) {
            a->frame++;
            a->frame_changed = true;
            continue;
        }

        if (clip->mode == GAMELIB_ANIM_LOOP) {
            a->frame = 0;
            a->frame_changed = true;
        } else {
            a->frame = clip->frame_count - 1;
            a->finished = true;
        }
    }
}

int gamelib_animator_frame(const gamelib_animator_t *a)
{
    return a ? a->frame : 0;
}

bool gamelib_animator_finished(const gamelib_animator_t *a)
{
    return a ? a->finished : true;
}

bool gamelib_animator_frame_changed(const gamelib_animator_t *a)
{
    return a ? a->frame_changed : false;
}

const gamelib_anim_clip_t *gamelib_animator_clip(const gamelib_animator_t *a)
{
    return a ? a->clip : NULL;
}
