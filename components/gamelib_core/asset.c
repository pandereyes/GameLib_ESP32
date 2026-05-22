#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

static void *asset_alloc(size_t size)
{
#ifdef ESP_PLATFORM
    void *buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buf) return buf;
#endif
    return malloc(size);
}

void *gamelib_file_load(gamelib_t *g, const char *path, size_t *out_len)
{
    if (out_len) *out_len = 0;
    if (!path) return NULL;

    void *file = gamelib_file_open(g, path, "rb");
    if (!file) return NULL;

    int size = gamelib_file_size(g, file);
    if (size <= 0) {
        gamelib_file_close(g, file);
        return NULL;
    }

    uint8_t *buf = (uint8_t*)asset_alloc((size_t)size);
    if (!buf) {
        gamelib_file_close(g, file);
        return NULL;
    }

    int read_len = gamelib_file_read(g, file, buf, size);
    gamelib_file_close(g, file);
    if (read_len != size) {
        free(buf);
        return NULL;
    }

    if (out_len) *out_len = (size_t)size;
    return buf;
}

void gamelib_file_free(void *data)
{
    free(data);
}

int gamelib_sprite_load_png_file(gamelib_t *g, const char *path)
{
    size_t len = 0;
    uint8_t *data = (uint8_t*)gamelib_file_load(g, path, &len);
    if (!data) return -1;
    int id = gamelib_sprite_load_png(g, data, len);
    gamelib_file_free(data);
    return id;
}

int gamelib_sprite_load_bmp_file(gamelib_t *g, const char *path)
{
    size_t len = 0;
    uint8_t *data = (uint8_t*)gamelib_file_load(g, path, &len);
    if (!data) return -1;
    int id = gamelib_sprite_load_bmp(g, data, len);
    gamelib_file_free(data);
    return id;
}

int gamelib_sprite_load_image_file(gamelib_t *g, const char *path)
{
    if (!path) return -1;
    const char *dot = strrchr(path, '.');
    if (dot) {
        if (strcmp(dot, ".png") == 0 || strcmp(dot, ".PNG") == 0) {
            return gamelib_sprite_load_png_file(g, path);
        }
        if (strcmp(dot, ".bmp") == 0 || strcmp(dot, ".BMP") == 0) {
            return gamelib_sprite_load_bmp_file(g, path);
        }
    }

    int id = gamelib_sprite_load_png_file(g, path);
    if (id >= 0) return id;
    return gamelib_sprite_load_bmp_file(g, path);
}
