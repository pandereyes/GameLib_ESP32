#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int   width;
    int   height;
    int   bpp;

    int  (*init)(void);
    void (*deinit)(void);
    void (*flush)(const void *fb, int x, int y, int w, int h);
    void (*wait_vsync)(void);
    void (*backlight)(uint8_t level);

    /* optional: DMA-capable framebuffer allocator */
    void *(*alloc_fb)(size_t size);
    void (*free_fb)(void *fb);
} hal_display_t;

#endif
