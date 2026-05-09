#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint16_t gamelib_color_t;

#define COLOR_NONE      0x0000
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0x00F8
#define COLOR_GREEN     0xE007
#define COLOR_BLUE      0x1F00
#define COLOR_YELLOW    0xE0FF
#define COLOR_CYAN      0xFF07
#define COLOR_MAGENTA   0x1FF8
#define COLOR_GRAY      0x1084
#define COLOR_DARKGRAY  0x0842
#define COLOR_LIGHTGRAY 0x18C6

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
