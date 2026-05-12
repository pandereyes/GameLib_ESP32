#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint16_t gamelib_color_t;

#define COLOR_NONE      0x0000
#define COLOR_BLACK     0x0000
#define COLOR_RED       0x00F8
#define COLOR_GREEN     0xE007
#define COLOR_BLUE      0x1F00
#define COLOR_YELLOW    0xE0FF
#define COLOR_CYAN      0xFF07
#define COLOR_MAGENTA   0x1FF8
#define COLOR_GRAY      0x1084
#define COLOR_DARKGRAY  0x0842
#define COLOR_LIGHTGRAY 0x18C6
/* Extended palette — all values byte-swapped RGB565 for SPI DMA */
#define COLOR_NAVY        0x1000
#define COLOR_DARK_GREEN  0x2003
#define COLOR_DARK_CYAN   0x5104
#define COLOR_MAROON      0x0080
#define COLOR_PURPLE      0x1080
#define COLOR_OLIVE       0x0084
#define COLOR_ORANGE      0x20FD
#define COLOR_GREEN_YELLOW 0xE5AF
#define COLOR_PINK        0x19FE
#define COLOR_BROWN       0x228A
#define COLOR_GOLD        0xA0FE
#define COLOR_DARK_BLUE   0x1100
#define COLOR_SKY_BLUE    0x7D86
#define COLOR_DARK_RED    0x0088
#define COLOR_WHITE     0xFFFF
/* Compose color from 8-bit RGB channels (byte-swapped for SPI DMA) */
#define COLOR_RGB(r, g, b) \
    ((uint16_t)(((r) & 0xF8) | ((g) >> 5)) | \
     ((uint16_t)(((g) & 0x1C) << 3) | ((b) >> 3)) << 8)

typedef enum {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_A,
    KEY_B,
    KEY_START,
    KEY_SELECT,
    KEY_COUNT
} hal_key_t;

#endif
