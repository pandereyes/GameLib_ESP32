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
} hal_audio_t;

#endif
