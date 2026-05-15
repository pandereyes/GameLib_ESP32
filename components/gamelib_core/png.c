#include "gamelib.h"
#include "lodepng.h"
#include <stdlib.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

int gamelib_sprite_load_png(gamelib_t *g, const uint8_t *data, size_t len)
{
    unsigned char *decoded = NULL;
    unsigned width = 0, height = 0;

    if (!data || len == 0) return -1;

    unsigned error = lodepng_decode32(&decoded, &width, &height, data, len);
    if (error) {
        if (decoded) free(decoded);
        return -1;
    }

    int sprite_id = gamelib_sprite_create(g, (int)width, (int)height);
    if (sprite_id < 0) {
        free(decoded);
        return -1;
    }

    /* 分配 alpha 数组，优先 PSRAM 回退 malloc */
    size_t pixel_count = (size_t)width * height;
#ifdef ESP_PLATFORM
    g->sprites[sprite_id].alpha = (uint8_t *)heap_caps_malloc(pixel_count, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g->sprites[sprite_id].alpha)
        g->sprites[sprite_id].alpha = (uint8_t *)malloc(pixel_count);
#else
    g->sprites[sprite_id].alpha = (uint8_t *)malloc(pixel_count);
#endif
    if (!g->sprites[sprite_id].alpha) {
        free(decoded);
        gamelib_sprite_free(g, sprite_id);
        return -1;
    }

    /* 逐像素转换 ARGB8888 → RGB565 + Alpha */
    bool has_transparent_pixels = false;
    for (unsigned i = 0; i < pixel_count; i++) {
        uint8_t p_r = decoded[i * 4 + 0];
        uint8_t p_g = decoded[i * 4 + 1];
        uint8_t p_b = decoded[i * 4 + 2];
        uint8_t p_a = decoded[i * 4 + 3];

        g->sprites[sprite_id].alpha[i] = p_a;

        if (p_a < 255) {
            has_transparent_pixels = true;
        }

        if (p_a == 0) {
            g->sprites[sprite_id].pixels[i] = COLOR_NONE;
        } else {
            gamelib_color_t c = ((p_r >> 3) << 11) | ((p_g >> 2) << 5) | (p_b >> 3);
            c = (c >> 8) | (c << 8);
            g->sprites[sprite_id].pixels[i] = c;
        }
    }

    free(decoded);

    /* 如果图片没有任何Alpha通道透明信息，则以左上角像素作为透明色(类似BMP)，否则使用COLOR_NONE */
    if (!has_transparent_pixels && pixel_count > 0) {
        gamelib_sprite_set_color_key(g, sprite_id, g->sprites[sprite_id].pixels[0]);
    } else {
        /* 自动设置 color key 兼容 SPRITE_COLORKEY 模式 */
        gamelib_sprite_set_color_key(g, sprite_id, COLOR_NONE);
    }

    return sprite_id;
}
