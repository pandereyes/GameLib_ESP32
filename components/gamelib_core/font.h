#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stdbool.h>

#include "stb_truetype.h"

#define MAX_FONTS 4

typedef struct {
    stbtt_fontinfo info;
    const uint8_t *ttf_buffer;
    int            font_size;
    float          scale;
    int            ascent;
    int            descent;
    uint8_t       *glyph_cache[256];
    bool           used;
} font_t;

#endif
