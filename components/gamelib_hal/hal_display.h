#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include <stdint.h>

typedef struct {
    int   width;
    int   height;
    int   bpp;

    int  (*init)(void);
    void (*deinit)(void);
    void (*flush)(const void *fb, int x, int y, int w, int h);
    void (*wait_vsync)(void);
    void (*backlight)(uint8_t level);
} hal_display_t;

#endif
