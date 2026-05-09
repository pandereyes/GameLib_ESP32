#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

/* BMP file header: 14 bytes */
#pragma pack(push, 1)
typedef struct {
    uint8_t  signature[2];   /* "BM" */
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_offset;
} bmp_file_header_t;

/* DIB header: BITMAPINFOHEADER, 40 bytes */
typedef struct {
    uint32_t header_size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_dib_header_t;
#pragma pack(pop)

int gamelib_sprite_load_bmp(gamelib_t *g, const uint8_t *data, size_t len)
{
    if (!data || len < 54) return -1;

    const bmp_file_header_t *fh = (const bmp_file_header_t *)data;
    if (fh->signature[0] != 'B' || fh->signature[1] != 'M') return -1;

    const bmp_dib_header_t *dib = (const bmp_dib_header_t *)(data + 14);
    if (dib->header_size < 40) return -1;
    if (dib->compression != 0) return -1;  /* only uncompressed */

    int width = dib->width;
    int height = dib->height;
    if (height < 0) height = -height;  /* top-down (rare) */
    int bpp = dib->bpp;

    int sprite_id = gamelib_sprite_create(g, width, height);
    if (sprite_id < 0) return -1;

    /* row size in bytes, padded to 4 bytes */
    int row_bytes_raw = (width * bpp + 7) / 8;
    int row_bytes = (row_bytes_raw + 3) & ~3;
    int bytes_per_pixel = (bpp + 7) / 8;

    const uint8_t *pixel_data = data + fh->data_offset;

    /* BMP stores rows bottom-up */
    for (int row = 0; row < height; row++) {
        const uint8_t *src_row = pixel_data + (height - 1 - row) * row_bytes;
        for (int col = 0; col < width; col++) {
            const uint8_t *px = src_row + col * bytes_per_pixel;
            gamelib_color_t c;
            if (bpp == 24) {
                /* BGR888 -> RGB565 */
                uint8_t b = px[0], g = px[1], r = px[2];
                c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                c = (c >> 8) | (c << 8);
            } else if (bpp == 32) {
                /* BGRA8888 -> RGB565 */
                uint8_t b = px[0], g = px[1], r = px[2];
                c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                c = (c >> 8) | (c << 8);
            } else if (bpp == 16) {
                /* Already RGB565 (or BGR555, handle common case) */
                c = ((gamelib_color_t)px[1] << 8) | px[0];
                c = (c >> 8) | (c << 8);
            } else {
                c = COLOR_MAGENTA;  /* unsupported format indicator */
            }
            gamelib_sprite_set_pixel(g, sprite_id, col, row, c);
        }
    }
    return sprite_id;
}
