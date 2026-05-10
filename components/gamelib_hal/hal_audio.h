#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int  (*init)(void);
    void (*deinit)(void);
    void (*beep)(int freq_hz, int duration_ms);
    void (*stop)(void);
    bool (*is_busy)(void);
    /* PCM output */
    int  (*play_pcm)(const uint8_t *data, int len, int sample_rate, int bits, int channels);
    void (*stop_pcm)(void);
    bool (*is_pcm_playing)(void);
    void (*set_volume)(int vol);
} hal_audio_t;

#endif
