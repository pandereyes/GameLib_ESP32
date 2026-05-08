# GameLib ESP32-S3 移植实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 GameLib 游戏引擎移植到 ESP32-S3，实现分层 HAL 架构的 MVE（基础绘图 + 精灵 + 输入 + PWM 蜂鸣）

**Architecture:** 三层分离 — gamelib_hal（接口定义）、gamelib_core（纯逻辑）、gamelib_port_esp32s3（硬件实现），通过组件依赖链组装

**Tech Stack:** C (C99), ESP-IDF, FreeRTOS, esp_lcd_panel, LEDC PWM, QMI8658

---

## Phase A: HAL 接口组件

### Task A1: 创建 gamelib_hal component 骨架

**Files:**
- Create: `components/gamelib_hal/CMakeLists.txt`
- Create: `components/gamelib_hal/hal_types.h`
- Create: `components/gamelib_hal/hal_display.h`
- Create: `components/gamelib_hal/hal_input.h`
- Create: `components/gamelib_hal/hal_audio.h`
- Create: `components/gamelib_hal/hal_timer.h`

- [ ] **Step 1: 创建目录和 CMakeLists.txt**

`components/gamelib_hal/CMakeLists.txt`:
```cmake
idf_component_register(
    INCLUDE_DIRS "."
    REQUIRES ""
)
```

- [ ] **Step 2: 创建 hal_types.h**

`components/gamelib_hal/hal_types.h`:
```c
#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint16_t gamelib_color_t;

#define COLOR_NONE      0x0000
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208
#define COLOR_LIGHTGRAY 0xC618

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
```

- [ ] **Step 3: 创建 hal_display.h**

`components/gamelib_hal/hal_display.h`:
```c
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
```

- [ ] **Step 4: 创建 hal_input.h**

`components/gamelib_hal/hal_input.h`:
```c
#ifndef HAL_INPUT_H
#define HAL_INPUT_H

#include "hal_types.h"

typedef struct {
    void (*init)(void);
    void (*poll)(void);
    bool (*is_down)(hal_key_t key);
    bool (*is_pressed)(hal_key_t key);
    bool (*is_released)(hal_key_t key);
    int  (*mouse_x)(void);
    int  (*mouse_y)(void);
    bool (*mouse_down)(int button);
} hal_input_t;

#endif
```

- [ ] **Step 5: 创建 hal_audio.h**

`components/gamelib_hal/hal_audio.h`:
```c
#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

#include <stdint.h>

typedef struct {
    int  (*init)(void);
    void (*deinit)(void);
    void (*beep)(int freq_hz, int duration_ms);
    void (*stop)(void);
    bool (*is_busy)(void);
} hal_audio_t;

#endif
```

- [ ] **Step 6: 创建 hal_timer.h**

`components/gamelib_hal/hal_timer.h`:
```c
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <stdint.h>

typedef struct {
    void     (*init)(void);
    uint32_t (*millis)(void);
    uint32_t (*micros)(void);
    void     (*delay_ms)(uint32_t ms);
    void     (*sleep_ms)(uint32_t ms);
} hal_timer_t;

#endif
```

- [ ] **Step 7: Commit**

```bash
git add components/gamelib_hal/CMakeLists.txt components/gamelib_hal/hal_types.h components/gamelib_hal/hal_display.h components/gamelib_hal/hal_input.h components/gamelib_hal/hal_audio.h components/gamelib_hal/hal_timer.h
git commit -m "feat: add gamelib_hal component with display/input/audio/timer interfaces"
```

---

## Phase B: gamelib_core 组件

### Task B1: 创建 gamelib_core component 骨架 + gamelib.h 公开头文件

**Files:**
- Create: `components/gamelib_core/CMakeLists.txt`
- Create: `components/gamelib_core/gamelib.h`

- [ ] **Step 1: 创建目录和 CMakeLists.txt**

`components/gamelib_core/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "gamelib.c" "framebuffer.c" "draw.c" "text.c" "sprite.c" "bmp.c" "audio.c"
    INCLUDE_DIRS "."
    REQUIRES gamelib_hal
)
```

- [ ] **Step 2: 创建 gamelib.h**

`components/gamelib_core/gamelib.h`:
```c
#ifndef GAMELIB_H
#define GAMELIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"
#include "hal_display.h"
#include "hal_input.h"
#include "hal_audio.h"
#include "hal_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SPRITES 64

/* --- sprite --- */
typedef struct {
    gamelib_color_t *pixels;
    int  width;
    int  height;
    bool used;
    gamelib_color_t color_key;
    bool has_color_key;
} sprite_t;

/* --- framebuffer --- */
typedef struct {
    gamelib_color_t *pixels;
    int width;
    int height;
    int clip_x, clip_y;
    int clip_w, clip_h;
} framebuffer_t;

/* --- main context --- */
typedef struct {
    framebuffer_t fb;
    bool          running;
    double        delta_time;
    double        fps;
    double        frame_start;
    int           target_fps;

    sprite_t      sprites[MAX_SPRITES];

    uint8_t       keystate[KEY_COUNT];
    uint8_t       key_prev[KEY_COUNT];
    int           mouse_x;
    int           mouse_y;
} gamelib_t;

/* --- HAL binding --- */
typedef struct {
    hal_display_t display;
    hal_input_t   input;
    hal_audio_t   audio;
    hal_timer_t   timer;
} gamelib_hal_t;

extern gamelib_hal_t g_hal;

/* --- lifecycle --- */
int    gamelib_init(gamelib_t *g, int w, int h, int target_fps);
void   gamelib_deinit(gamelib_t *g);
bool   gamelib_is_closed(gamelib_t *g);
void   gamelib_update(gamelib_t *g);
void   gamelib_wait_frame(gamelib_t *g);
double gamelib_get_fps(gamelib_t *g);

/* --- drawing --- */
void gamelib_clear(gamelib_t *g, gamelib_color_t color);
void gamelib_set_pixel(gamelib_t *g, int x, int y, gamelib_color_t c);
void gamelib_draw_line(gamelib_t *g, int x1,int y1,int x2,int y2, gamelib_color_t c);
void gamelib_draw_rect(gamelib_t *g, int x,int y,int w,int h, gamelib_color_t c);
void gamelib_fill_rect(gamelib_t *g, int x,int y,int w,int h, gamelib_color_t c);
void gamelib_draw_circle(gamelib_t *g, int cx,int cy,int r, gamelib_color_t c);
void gamelib_fill_circle(gamelib_t *g, int cx,int cy,int r, gamelib_color_t c);
void gamelib_draw_ellipse(gamelib_t *g, int cx,int cy,int rx,int ry, gamelib_color_t c);
void gamelib_fill_ellipse(gamelib_t *g, int cx,int cy,int rx,int ry, gamelib_color_t c);
void gamelib_draw_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c);
void gamelib_fill_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c);

/* --- clip --- */
void gamelib_set_clip(gamelib_t *g, int x, int y, int w, int h);
void gamelib_clear_clip(gamelib_t *g);

/* --- text --- */
void gamelib_draw_text(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c);
void gamelib_draw_text_scale(gamelib_t *g, int x,int y, const char *s,
                             gamelib_color_t c, int scale_w, int scale_h);
void gamelib_draw_number(gamelib_t *g, int x, int y, int n, gamelib_color_t c);

/* --- sprite --- */
int  gamelib_sprite_create(gamelib_t *g, int w, int h);
int  gamelib_sprite_load_bmp(gamelib_t *g, const uint8_t *data, size_t len);
void gamelib_sprite_free(gamelib_t *g, int id);
void gamelib_sprite_set_pixel(gamelib_t *g, int id, int x, int y, gamelib_color_t c);
void gamelib_draw_sprite(gamelib_t *g, int id, int x, int y);
void gamelib_draw_sprite_region(gamelib_t *g, int id, int x,int y,
                                int sx,int sy,int sw,int sh);
int  gamelib_sprite_width(gamelib_t *g, int id);
int  gamelib_sprite_height(gamelib_t *g, int id);
void gamelib_sprite_set_color_key(gamelib_t *g, int id, gamelib_color_t c);

/* --- input --- */
bool gamelib_is_key_down(gamelib_t *g, int key);
bool gamelib_is_key_pressed(gamelib_t *g, int key);
bool gamelib_is_key_released(gamelib_t *g, int key);
int  gamelib_mouse_x(gamelib_t *g);
int  gamelib_mouse_y(gamelib_t *g);

/* --- audio --- */
void gamelib_play_beep(gamelib_t *g, int freq, int duration_ms);
void gamelib_stop_beep(gamelib_t *g);

/* --- helpers --- */
int  gamelib_random(int minVal, int maxVal);
bool gamelib_rect_overlap(int x1,int y1,int w1,int h1, int x2,int y2,int w2,int h2);
bool gamelib_point_in_rect(int px,int py, int x,int y,int w,int h);
float gamelib_distance_f(int x1,int y1, int x2,int y2);

#ifdef __cplusplus
}
#endif

#endif
```

- [ ] **Step 3: Commit**

```bash
git add components/gamelib_core/CMakeLists.txt components/gamelib_core/gamelib.h
git commit -m "feat: add gamelib_core component skeleton and public header"
```

---

### Task B2: 创建 font_8x8.h 内置字体

**Files:**
- Create: `components/gamelib_core/font_8x8.h`

- [ ] **Step 1: 创建 font_8x8.h**

`components/gamelib_core/font_8x8.h`:
```c
#ifndef FONT_8X8_H
#define FONT_8X8_H

#include <stdint.h>

static const uint8_t font_8x8[128][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0 NUL */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 2 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 3 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 4 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 6 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 7 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 8 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 10 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 11 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 12 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 13 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 14 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 15 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 16 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 17 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 18 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 19 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 20 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 21 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 22 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 23 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 24 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 25 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 26 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 27 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 28 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 29 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 30 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 31 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 32 ' ' */
    {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00}, /* 33 '!' */
    {0x00,0x07,0x00,0x07,0x00,0x00,0x00,0x00}, /* 34 '"' */
    {0x14,0x7F,0x14,0x7F,0x14,0x00,0x00,0x00}, /* 35 '#' */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00,0x00,0x00}, /* 36 '$' */
    {0x23,0x13,0x08,0x64,0x62,0x00,0x00,0x00}, /* 37 '%' */
    {0x36,0x49,0x55,0x22,0x50,0x00,0x00,0x00}, /* 38 '&' */
    {0x00,0x05,0x03,0x00,0x00,0x00,0x00,0x00}, /* 39 ''' */
    {0x00,0x1C,0x22,0x41,0x00,0x00,0x00,0x00}, /* 40 '(' */
    {0x00,0x41,0x22,0x1C,0x00,0x00,0x00,0x00}, /* 41 ')' */
    {0x08,0x2A,0x1C,0x2A,0x08,0x00,0x00,0x00}, /* 42 '*' */
    {0x08,0x08,0x3E,0x08,0x08,0x00,0x00,0x00}, /* 43 '+' */
    {0x00,0x50,0x30,0x00,0x00,0x00,0x00,0x00}, /* 44 ',' */
    {0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00}, /* 45 '-' */
    {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00}, /* 46 '.' */
    {0x20,0x10,0x08,0x04,0x02,0x00,0x00,0x00}, /* 47 '/' */
    {0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00}, /* 48 '0' */
    {0x00,0x42,0x7F,0x40,0x00,0x00,0x00,0x00}, /* 49 '1' */
    {0x42,0x61,0x51,0x49,0x46,0x00,0x00,0x00}, /* 50 '2' */
    {0x21,0x41,0x45,0x4B,0x31,0x00,0x00,0x00}, /* 51 '3' */
    {0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00}, /* 52 '4' */
    {0x27,0x45,0x45,0x45,0x39,0x00,0x00,0x00}, /* 53 '5' */
    {0x3C,0x4A,0x49,0x49,0x30,0x00,0x00,0x00}, /* 54 '6' */
    {0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00}, /* 55 '7' */
    {0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00}, /* 56 '8' */
    {0x06,0x49,0x49,0x29,0x1E,0x00,0x00,0x00}, /* 57 '9' */
    {0x00,0x36,0x36,0x00,0x00,0x00,0x00,0x00}, /* 58 ':' */
    {0x00,0x56,0x36,0x00,0x00,0x00,0x00,0x00}, /* 59 ';' */
    {0x00,0x08,0x14,0x22,0x41,0x00,0x00,0x00}, /* 60 '<' */
    {0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00}, /* 61 '=' */
    {0x41,0x22,0x14,0x08,0x00,0x00,0x00,0x00}, /* 62 '>' */
    {0x02,0x01,0x51,0x09,0x06,0x00,0x00,0x00}, /* 63 '?' */
    {0x32,0x49,0x79,0x41,0x3E,0x00,0x00,0x00}, /* 64 '@' */
    {0x7E,0x11,0x11,0x11,0x7E,0x00,0x00,0x00}, /* 65 'A' */
    {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00}, /* 66 'B' */
    {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00}, /* 67 'C' */
    {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00}, /* 68 'D' */
    {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00}, /* 69 'E' */
    {0x7F,0x09,0x09,0x01,0x01,0x00,0x00,0x00}, /* 70 'F' */
    {0x3E,0x41,0x41,0x51,0x32,0x00,0x00,0x00}, /* 71 'G' */
    {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00}, /* 72 'H' */
    {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00}, /* 73 'I' */
    {0x20,0x40,0x41,0x3F,0x01,0x00,0x00,0x00}, /* 74 'J' */
    {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00}, /* 75 'K' */
    {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, /* 76 'L' */
    {0x7F,0x02,0x04,0x02,0x7F,0x00,0x00,0x00}, /* 77 'M' */
    {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00}, /* 78 'N' */
    {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00}, /* 79 'O' */
    {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00}, /* 80 'P' */
    {0x3E,0x41,0x51,0x21,0x5E,0x00,0x00,0x00}, /* 81 'Q' */
    {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00}, /* 82 'R' */
    {0x46,0x49,0x49,0x49,0x31,0x00,0x00,0x00}, /* 83 'S' */
    {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00}, /* 84 'T' */
    {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00}, /* 85 'U' */
    {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00}, /* 86 'V' */
    {0x7F,0x20,0x18,0x20,0x7F,0x00,0x00,0x00}, /* 87 'W' */
    {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00}, /* 88 'X' */
    {0x03,0x04,0x78,0x04,0x03,0x00,0x00,0x00}, /* 89 'Y' */
    {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00}, /* 90 'Z' */
    {0x00,0x00,0x7F,0x41,0x41,0x00,0x00,0x00}, /* 91 '[' */
    {0x02,0x04,0x08,0x10,0x20,0x00,0x00,0x00}, /* 92 '\' */
    {0x41,0x41,0x7F,0x00,0x00,0x00,0x00,0x00}, /* 93 ']' */
    {0x04,0x02,0x01,0x02,0x04,0x00,0x00,0x00}, /* 94 '^' */
    {0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, /* 95 '_' */
    {0x00,0x01,0x02,0x04,0x00,0x00,0x00,0x00}, /* 96 '`' */
    {0x20,0x54,0x54,0x54,0x78,0x00,0x00,0x00}, /* 97 'a' */
    {0x7F,0x48,0x44,0x44,0x38,0x00,0x00,0x00}, /* 98 'b' */
    {0x38,0x44,0x44,0x44,0x20,0x00,0x00,0x00}, /* 99 'c' */
    {0x38,0x44,0x44,0x48,0x7F,0x00,0x00,0x00}, /* 100 'd' */
    {0x38,0x54,0x54,0x54,0x18,0x00,0x00,0x00}, /* 101 'e' */
    {0x08,0x7E,0x09,0x01,0x02,0x00,0x00,0x00}, /* 102 'f' */
    {0x08,0x14,0x54,0x54,0x3C,0x00,0x00,0x00}, /* 103 'g' */
    {0x7F,0x08,0x04,0x04,0x78,0x00,0x00,0x00}, /* 104 'h' */
    {0x00,0x44,0x7D,0x40,0x00,0x00,0x00,0x00}, /* 105 'i' */
    {0x20,0x40,0x44,0x3D,0x00,0x00,0x00,0x00}, /* 106 'j' */
    {0x00,0x7F,0x10,0x28,0x44,0x00,0x00,0x00}, /* 107 'k' */
    {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00}, /* 108 'l' */
    {0x7C,0x04,0x18,0x04,0x78,0x00,0x00,0x00}, /* 109 'm' */
    {0x7C,0x08,0x04,0x04,0x78,0x00,0x00,0x00}, /* 110 'n' */
    {0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00}, /* 111 'o' */
    {0x7C,0x14,0x14,0x14,0x08,0x00,0x00,0x00}, /* 112 'p' */
    {0x08,0x14,0x14,0x18,0x7C,0x00,0x00,0x00}, /* 113 'q' */
    {0x7C,0x08,0x04,0x04,0x08,0x00,0x00,0x00}, /* 114 'r' */
    {0x48,0x54,0x54,0x54,0x20,0x00,0x00,0x00}, /* 115 's' */
    {0x04,0x3F,0x44,0x40,0x20,0x00,0x00,0x00}, /* 116 't' */
    {0x3C,0x40,0x40,0x20,0x7C,0x00,0x00,0x00}, /* 117 'u' */
    {0x1C,0x20,0x40,0x20,0x1C,0x00,0x00,0x00}, /* 118 'v' */
    {0x3C,0x40,0x30,0x40,0x3C,0x00,0x00,0x00}, /* 119 'w' */
    {0x44,0x28,0x10,0x28,0x44,0x00,0x00,0x00}, /* 120 'x' */
    {0x0C,0x50,0x50,0x50,0x3C,0x00,0x00,0x00}, /* 121 'y' */
    {0x44,0x64,0x54,0x4C,0x44,0x00,0x00,0x00}, /* 122 'z' */
    {0x00,0x08,0x36,0x41,0x00,0x00,0x00,0x00}, /* 123 '{' */
    {0x00,0x00,0x7F,0x00,0x00,0x00,0x00,0x00}, /* 124 '|' */
    {0x00,0x41,0x36,0x08,0x00,0x00,0x00,0x00}, /* 125 '}' */
    {0x08,0x08,0x2A,0x1C,0x08,0x00,0x00,0x00}, /* 126 '~' */
    {0x08,0x1C,0x2A,0x08,0x08,0x00,0x00,0x00}, /* 127 DEL */
};
#endif
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/font_8x8.h
git commit -m "feat: add 8x8 bitmap font data"
```

---

### Task B3: framebuffer.c + draw.c — framebuffer 操作和绘图算法

**Files:**
- Create: `components/gamelib_core/framebuffer.c`

- [ ] **Step 1: 创建 framebuffer.c**

`components/gamelib_core/framebuffer.c`:
```c
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

static inline void fb_set_pixel(framebuffer_t *fb, int x, int y, gamelib_color_t c)
{
    if (x >= fb->clip_x && x < fb->clip_x + fb->clip_w &&
        y >= fb->clip_y && y < fb->clip_y + fb->clip_h) {
        fb->pixels[y * fb->width + x] = c;
    }
}

void gamelib_clear(gamelib_t *g, gamelib_color_t color)
{
    int total = g->fb.width * g->fb.height;
    for (int i = 0; i < total; i++) {
        g->fb.pixels[i] = color;
    }
}

void gamelib_set_pixel(gamelib_t *g, int x, int y, gamelib_color_t c)
{
    fb_set_pixel(&g->fb, x, y, c);
}

void gamelib_set_clip(gamelib_t *g, int x, int y, int w, int h)
{
    g->fb.clip_x = x;
    g->fb.clip_y = y;
    g->fb.clip_w = w;
    g->fb.clip_h = h;
}

void gamelib_clear_clip(gamelib_t *g)
{
    g->fb.clip_x = 0;
    g->fb.clip_y = 0;
    g->fb.clip_w = g->fb.width;
    g->fb.clip_h = g->fb.height;
}

static void fb_hline(framebuffer_t *fb, int x1, int x2, int y, gamelib_color_t c)
{
    if (y < fb->clip_y || y >= fb->clip_y + fb->clip_h) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < fb->clip_x) x1 = fb->clip_x;
    if (x2 >= fb->clip_x + fb->clip_w) x2 = fb->clip_x + fb->clip_w - 1;
    gamelib_color_t *row = fb->pixels + y * fb->width;
    for (int x = x1; x <= x2; x++) {
        row[x] = c;
    }
}
```

- [ ] **Step 2: 创建 draw.c**

`components/gamelib_core/draw.c`:
```c
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

static void fb_set_pixel(framebuffer_t *fb, int x, int y, gamelib_color_t c)
{
    if (x >= fb->clip_x && x < fb->clip_x + fb->clip_w &&
        y >= fb->clip_y && y < fb->clip_y + fb->clip_h) {
        fb->pixels[y * fb->width + x] = c;
    }
}

static void fb_hline(framebuffer_t *fb, int x1, int x2, int y, gamelib_color_t c)
{
    if (y < fb->clip_y || y >= fb->clip_y + fb->clip_h) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < fb->clip_x) x1 = fb->clip_x;
    if (x2 >= fb->clip_x + fb->clip_w) x2 = fb->clip_x + fb->clip_w - 1;
    if (x1 > x2) return;
    gamelib_color_t *row = fb->pixels + y * fb->width;
    for (int x = x1; x <= x2; x++) row[x] = c;
}

void gamelib_draw_line(gamelib_t *g, int x1, int y1, int x2, int y2, gamelib_color_t c)
{
    if (g->fb.clip_w <= 0 || g->fb.clip_h <= 0) return;
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        fb_set_pixel(&g->fb, x1, y1, c);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void gamelib_draw_rect(gamelib_t *g, int x, int y, int w, int h, gamelib_color_t c)
{
    if (w <= 0 || h <= 0) return;
    fb_hline(&g->fb, x, x + w - 1, y, c);
    fb_hline(&g->fb, x, x + w - 1, y + h - 1, c);
    for (int j = y + 1; j < y + h - 1; j++) {
        fb_set_pixel(&g->fb, x, j, c);
        fb_set_pixel(&g->fb, x + w - 1, j, c);
    }
}

void gamelib_fill_rect(gamelib_t *g, int x, int y, int w, int h, gamelib_color_t c)
{
    if (w <= 0 || h <= 0) return;
    int x1 = x, y1 = y, x2 = x + w, y2 = y + h;
    if (x1 < g->fb.clip_x) x1 = g->fb.clip_x;
    if (y1 < g->fb.clip_y) y1 = g->fb.clip_y;
    if (x2 > g->fb.clip_x + g->fb.clip_w) x2 = g->fb.clip_x + g->fb.clip_w;
    if (y2 > g->fb.clip_y + g->fb.clip_h) y2 = g->fb.clip_y + g->fb.clip_h;
    for (int j = y1; j < y2; j++) {
        fb_hline(&g->fb, x1, x2 - 1, j, c);
    }
}

void gamelib_draw_circle(gamelib_t *g, int cx, int cy, int r, gamelib_color_t c)
{
    if (r < 0) return;
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        fb_set_pixel(&g->fb, cx + x, cy + y, c);
        fb_set_pixel(&g->fb, cx + x, cy - y, c);
        fb_set_pixel(&g->fb, cx - x, cy + y, c);
        fb_set_pixel(&g->fb, cx - x, cy - y, c);
        fb_set_pixel(&g->fb, cx + y, cy + x, c);
        fb_set_pixel(&g->fb, cx + y, cy - x, c);
        fb_set_pixel(&g->fb, cx - y, cy + x, c);
        fb_set_pixel(&g->fb, cx - y, cy - x, c);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void gamelib_fill_circle(gamelib_t *g, int cx, int cy, int r, gamelib_color_t c)
{
    gamelib_fill_ellipse(g, cx, cy, r, r, c);
}

void gamelib_draw_ellipse(gamelib_t *g, int cx, int cy, int rx, int ry, gamelib_color_t c)
{
    if (rx < 0 || ry < 0) return;
    if (rx == 0 && ry == 0) {
        fb_set_pixel(&g->fb, cx, cy, c);
        return;
    }
    if (rx == 0) {
        gamelib_draw_line(g, cx, cy - ry, cx, cy + ry, c);
        return;
    }
    if (ry == 0) {
        gamelib_draw_line(g, cx - rx, cy, cx + rx, cy, c);
        return;
    }
    int rx2 = rx * rx, ry2 = ry * ry;
    int twoRx2 = 2 * rx2, twoRy2 = 2 * ry2;
    int x = 0, y = ry, px = 0, py = twoRx2 * y;
    double p = (double)ry2 - (double)rx2 * ry + 0.25 * rx2;
    while (px < py) {
        fb_set_pixel(&g->fb, cx + x, cy + y, c);
        fb_set_pixel(&g->fb, cx - x, cy + y, c);
        fb_set_pixel(&g->fb, cx + x, cy - y, c);
        fb_set_pixel(&g->fb, cx - x, cy - y, c);
        x++; px += twoRy2;
        if (p < 0.0) {
            p += ry2 + px;
        } else {
            y--; py -= twoRx2;
            p += ry2 + px - py;
        }
    }
    p = (double)ry2 * (x + 0.5) * (x + 0.5) +
        (double)rx2 * (y - 1.0) * (y - 1.0) -
        (double)rx2 * ry2;
    while (y >= 0) {
        fb_set_pixel(&g->fb, cx + x, cy + y, c);
        fb_set_pixel(&g->fb, cx - x, cy + y, c);
        fb_set_pixel(&g->fb, cx + x, cy - y, c);
        fb_set_pixel(&g->fb, cx - x, cy - y, c);
        y--; py -= twoRx2;
        if (p > 0.0) {
            x++; px += twoRy2;
            p += ry2 + px - py;
        } else {
            p += ry2 - py;
        }
    }
}

void gamelib_fill_ellipse(gamelib_t *g, int cx, int cy, int rx, int ry, gamelib_color_t c)
{
    if (rx < 0 || ry < 0) return;
    if (rx == 0 && ry == 0) {
        fb_set_pixel(&g->fb, cx, cy, c);
        return;
    }
    if (rx == 0) {
        gamelib_draw_line(g, cx, cy - ry, cx, cy + ry, c);
        return;
    }
    if (ry == 0) {
        gamelib_draw_line(g, cx - rx, cy, cx + rx, cy, c);
        return;
    }
    int rx2 = rx * rx, ry2 = ry * ry;
    int twoRx2 = 2 * rx2, twoRy2 = 2 * ry2;
    int x = 0, y = ry, px = 0, py = twoRx2 * y;
    double p = (double)ry2 - (double)rx2 * ry + 0.25 * rx2;
    while (px < py) {
        fb_hline(&g->fb, cx - x, cx + x, cy + y, c);
        if (y > 0) fb_hline(&g->fb, cx - x, cx + x, cy - y, c);
        x++; px += twoRy2;
        if (p < 0.0) {
            p += ry2 + px;
        } else {
            y--; py -= twoRx2;
            p += ry2 + px - py;
        }
    }
    p = (double)ry2 * (x + 0.5) * (x + 0.5) +
        (double)rx2 * (y - 1.0) * (y - 1.0) -
        (double)rx2 * ry2;
    while (y >= 0) {
        fb_hline(&g->fb, cx - x, cx + x, cy + y, c);
        if (y > 0) fb_hline(&g->fb, cx - x, cx + x, cy - y, c);
        y--; py -= twoRx2;
        if (p > 0.0) {
            x++; px += twoRy2;
            p += ry2 + px - py;
        } else {
            p += ry2 - py;
        }
    }
}

void gamelib_draw_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c)
{
    gamelib_draw_line(g, x1, y1, x2, y2, c);
    gamelib_draw_line(g, x2, y2, x3, y3, c);
    gamelib_draw_line(g, x3, y3, x1, y1, c);
}

void gamelib_fill_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c)
{
    if (y1 > y2) { int t; t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    if (y1 > y3) { int t; t = x1; x1 = x3; x3 = t; t = y1; y1 = y3; y3 = t; }
    if (y2 > y3) { int t; t = x2; x2 = x3; x3 = t; t = y2; y2 = y3; y3 = t; }
    if (y2 == y3) {
        if (x2 > x3) { int t = x2; x2 = x3; x3 = t; }
        for (int y = y1; y <= y3; y++) {
            float t = (y3 == y1) ? 0.0f : (float)(y - y1) / (y3 - y1);
            int xL = x1 + (int)(t * (x2 - x1));
            int xR = x1 + (int)(t * (x3 - x1));
            if (xL > xR) { int tmp = xL; xL = xR; xR = tmp; }
            fb_hline(&g->fb, xL, xR, y, c);
        }
    } else {
        for (int y = y1; y <= y2; y++) {
            float t = (y2 == y1) ? 0.0f : (float)(y - y1) / (y2 - y1);
            int xL = x1 + (int)(t * (x2 - x1));
            int xR = x1 + (int)(t * (x3 - x1));
            if (xL > xR) { int tmp = xL; xL = xR; xR = tmp; }
            fb_hline(&g->fb, xL, xR, y, c);
        }
        for (int y = y2; y <= y3; y++) {
            float t = (y3 == y2) ? 0.0f : (float)(y - y2) / (y3 - y2);
            int xL = x2 + (int)(t * (x3 - x2));
            int xR = x1 + (int)((float)(y - y1) / (y3 - y1) * (x3 - x1));
            if (xL > xR) { int tmp = xL; xL = xR; xR = tmp; }
            fb_hline(&g->fb, xL, xR, y, c);
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/framebuffer.c components/gamelib_core/draw.c
git commit -m "feat: add framebuffer ops and drawing primitives"
```

---

### Task B4: text.c — 内置 8x8 文字渲染

**Files:**
- Create: `components/gamelib_core/text.c`

- [ ] **Step 1: 创建 text.c**

`components/gamelib_core/text.c`:
```c
#include "gamelib.h"
#include "font_8x8.h"

static void draw_char(gamelib_t *g, int x, int y, char ch, gamelib_color_t c, int sw, int sh)
{
    const uint8_t *glyph = font_8x8[(unsigned char)ch];
    framebuffer_t *fb = &g->fb;
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                int px = x + col * sw;
                int py = y + row * sh;
                for (int sy = 0; sy < sh; sy++) {
                    for (int sx = 0; sx < sw; sx++) {
                        int fx = px + sx;
                        int fy = py + sy;
                        if (fx >= fb->clip_x && fx < fb->clip_x + fb->clip_w &&
                            fy >= fb->clip_y && fy < fb->clip_y + fb->clip_h) {
                            fb->pixels[fy * fb->width + fx] = c;
                        }
                    }
                }
            }
        }
    }
}

void gamelib_draw_text(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c)
{
    while (*s) {
        draw_char(g, x, y, *s, c, 1, 1);
        x += 8;
        s++;
    }
}

void gamelib_draw_text_scale(gamelib_t *g, int x, int y, const char *s,
                             gamelib_color_t c, int scale_w, int scale_h)
{
    if (scale_w < 1) scale_w = 1;
    if (scale_h < 1) scale_h = 1;
    while (*s) {
        draw_char(g, x, y, *s, c, scale_w, scale_h);
        x += 8 * scale_w;
        s++;
    }
}

void gamelib_draw_number(gamelib_t *g, int x, int y, int n, gamelib_color_t c)
{
    char buf[16];
    int i = 0;
    if (n < 0) {
        buf[i++] = '-';
        n = -n;
    }
    if (n == 0) {
        buf[i++] = '0';
    } else {
        int start = i;
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        for (int j = start, k = i - 1; j < k; j++, k--) {
            char t = buf[j];
            buf[j] = buf[k];
            buf[k] = t;
        }
    }
    buf[i] = '\0';
    gamelib_draw_text(g, x, y, buf, c);
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/text.c
git commit -m "feat: add 8x8 bitmap text rendering"
```

---

### Task B5: sprite.c — 精灵管理

**Files:**
- Create: `components/gamelib_core/sprite.c`

- [ ] **Step 1: 创建 sprite.c**

`components/gamelib_core/sprite.c`:
```c
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

int gamelib_sprite_create(gamelib_t *g, int w, int h)
{
    for (int i = 0; i < MAX_SPRITES; i++) {
        if (!g->sprites[i].used) {
            g->sprites[i].pixels = (gamelib_color_t*)calloc(w * h, sizeof(gamelib_color_t));
            if (!g->sprites[i].pixels) return -1;
            g->sprites[i].width = w;
            g->sprites[i].height = h;
            g->sprites[i].used = true;
            g->sprites[i].has_color_key = false;
            g->sprites[i].color_key = COLOR_NONE;
            return i;
        }
    }
    return -1;
}

void gamelib_sprite_free(gamelib_t *g, int id)
{
    if (id < 0 || id >= MAX_SPRITES) return;
    if (g->sprites[id].used) {
        free(g->sprites[id].pixels);
        g->sprites[id].pixels = NULL;
        g->sprites[id].used = false;
    }
}

void gamelib_sprite_set_pixel(gamelib_t *g, int id, int x, int y, gamelib_color_t c)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    if (x < 0 || x >= g->sprites[id].width || y < 0 || y >= g->sprites[id].height) return;
    g->sprites[id].pixels[y * g->sprites[id].width + x] = c;
}

int gamelib_sprite_width(gamelib_t *g, int id)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return 0;
    return g->sprites[id].width;
}

int gamelib_sprite_height(gamelib_t *g, int id)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return 0;
    return g->sprites[id].height;
}

void gamelib_sprite_set_color_key(gamelib_t *g, int id, gamelib_color_t c)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    g->sprites[id].color_key = c;
    g->sprites[id].has_color_key = true;
}

void gamelib_draw_sprite(gamelib_t *g, int id, int x, int y)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    gamelib_draw_sprite_region(g, id, x, y, 0, 0,
                               g->sprites[id].width, g->sprites[id].height);
}

void gamelib_draw_sprite_region(gamelib_t *g, int id, int dst_x, int dst_y,
                                int sx, int sy, int sw, int sh)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx + sw > sp->width)  sw = sp->width - sx;
    if (sy + sh > sp->height) sh = sp->height - sy;
    framebuffer_t *fb = &g->fb;
    for (int row = 0; row < sh; row++) {
        for (int col = 0; col < sw; col++) {
            gamelib_color_t c = sp->pixels[(sy + row) * sp->width + (sx + col)];
            if (sp->has_color_key && c == sp->color_key) continue;
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                fb->pixels[py * fb->width + px] = c;
            }
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/sprite.c
git commit -m "feat: add sprite management and drawing"
```

---

### Task B6: bmp.c — BMP 解码器

**Files:**
- Create: `components/gamelib_core/bmp.c`

- [ ] **Step 1: 创建 bmp.c**

`components/gamelib_core/bmp.c`:
```c
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
            } else if (bpp == 32) {
                /* BGRA8888 -> RGB565 */
                uint8_t b = px[0], g = px[1], r = px[2];
                c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            } else if (bpp == 16) {
                /* Already RGB565 (or BGR555, handle common case) */
                c = ((gamelib_color_t)px[1] << 8) | px[0];
            } else {
                c = COLOR_MAGENTA;  /* unsupported format indicator */
            }
            gamelib_sprite_set_pixel(g, sprite_id, col, row, c);
        }
    }
    return sprite_id;
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/bmp.c
git commit -m "feat: add BMP decoder from memory buffer"
```

---

### Task B7: audio.c — 音频封装

**Files:**
- Create: `components/gamelib_core/audio.c`

- [ ] **Step 1: 创建 audio.c**

`components/gamelib_core/audio.c`:
```c
#include "gamelib.h"

void gamelib_play_beep(gamelib_t *g, int freq, int duration_ms)
{
    (void)g;
    if (g_hal.audio.beep) {
        g_hal.audio.beep(freq, duration_ms);
    }
}

void gamelib_stop_beep(gamelib_t *g)
{
    (void)g;
    if (g_hal.audio.stop) {
        g_hal.audio.stop();
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/audio.c
git commit -m "feat: add audio HAL wrapper"
```

---

### Task B8: gamelib.c — 主上下文 + 游戏循环 + 输入

**Files:**
- Create: `components/gamelib_core/gamelib.c`

- [ ] **Step 1: 创建 gamelib.c**

`components/gamelib_core/gamelib.c`:
```c
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

gamelib_hal_t g_hal;

int gamelib_init(gamelib_t *g, int w, int h, int target_fps)
{
    memset(g, 0, sizeof(*g));
    g->fb.width = w;
    g->fb.height = h;
    g->fb.clip_x = 0;
    g->fb.clip_y = 0;
    g->fb.clip_w = w;
    g->fb.clip_h = h;
    g->target_fps = target_fps;

    g->fb.pixels = (gamelib_color_t*)calloc(w * h, sizeof(gamelib_color_t));
    if (!g->fb.pixels) return -1;

    if (g_hal.display.init) {
        if (g_hal.display.init() != 0) {
            free(g->fb.pixels);
            return -1;
        }
    }
    if (g_hal.input.init)  g_hal.input.init();
    if (g_hal.timer.init)  g_hal.timer.init();
    if (g_hal.audio.init)  g_hal.audio.init();

    g->running = true;
    g->frame_start = (double)g_hal.timer.micros() / 1000000.0;
    return 0;
}

void gamelib_deinit(gamelib_t *g)
{
    if (g_hal.display.deinit) g_hal.display.deinit();
    if (g_hal.audio.deinit)   g_hal.audio.deinit();
    for (int i = 0; i < MAX_SPRITES; i++) {
        gamelib_sprite_free(g, i);
    }
    free(g->fb.pixels);
    g->fb.pixels = NULL;
    g->running = false;
}

bool gamelib_is_closed(gamelib_t *g)
{
    return !g->running;
}

void gamelib_update(gamelib_t *g)
{
    if (!g->running) return;

    /* flush */
    if (g_hal.display.flush) {
        g_hal.display.flush(g->fb.pixels, 0, 0, g->fb.width, g->fb.height);
    }

    /* input */
    if (g_hal.input.poll) {
        g_hal.input.poll();
        for (int i = 0; i < KEY_COUNT; i++) {
            g->key_prev[i] = g->keystate[i];
            g->keystate[i] = g_hal.input.is_down((hal_key_t)i) ? 1 : 0;
        }
        g->mouse_x = g_hal.input.mouse_x ? g_hal.input.mouse_x() : 0;
        g->mouse_y = g_hal.input.mouse_y ? g_hal.input.mouse_y() : 0;
    }
}

void gamelib_wait_frame(gamelib_t *g)
{
    if (!g->running) return;
    double now = (double)g_hal.timer.micros() / 1000000.0;
    g->delta_time = now - g->frame_start;
    double target = 1.0 / (double)g->target_fps;
    if (g->delta_time < target) {
        uint32_t sleep_ms = (uint32_t)((target - g->delta_time) * 1000.0);
        g_hal.timer.sleep_ms(sleep_ms);
        now = (double)g_hal.timer.micros() / 1000000.0;
        g->delta_time = now - g->frame_start;
    }
    if (g->delta_time > 0.0) {
        g->fps = 1.0 / g->delta_time;
    }
    g->frame_start = now;
}

double gamelib_get_fps(gamelib_t *g)
{
    return g->fps;
}

/* --- input --- */
bool gamelib_is_key_down(gamelib_t *g, int key)
{
    if (key < 0 || key >= KEY_COUNT) return false;
    return g->keystate[key] != 0;
}

bool gamelib_is_key_pressed(gamelib_t *g, int key)
{
    if (key < 0 || key >= KEY_COUNT) return false;
    return g->keystate[key] != 0 && g->key_prev[key] == 0;
}

bool gamelib_is_key_released(gamelib_t *g, int key)
{
    if (key < 0 || key >= KEY_COUNT) return false;
    return g->keystate[key] == 0 && g->key_prev[key] != 0;
}

int gamelib_mouse_x(gamelib_t *g) { return g->mouse_x; }
int gamelib_mouse_y(gamelib_t *g) { return g->mouse_y; }

/* --- helpers --- */
int gamelib_random(int minVal, int maxVal)
{
    return minVal + (rand() % (maxVal - minVal + 1));
}

bool gamelib_rect_overlap(int x1,int y1,int w1,int h1, int x2,int y2,int w2,int h2)
{
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

bool gamelib_point_in_rect(int px,int py, int x,int y,int w,int h)
{
    return (px >= x && px < x + w && py >= y && py < y + h);
}

#include <math.h>
float gamelib_distance_f(int x1,int y1, int x2,int y2)
{
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    return sqrtf(dx * dx + dy * dy);
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_core/gamelib.c
git commit -m "feat: add game loop, input muxing, and helper functions"
```

---

## Phase C: ESP32-S3 Port 组件

### Task C1: 迁移 bsp_qmi8658 到 port component

**Files:**
- Copy: `main/bsp_qmi8658.h` → `components/gamelib_port_esp32s3/bsp_qmi8658.h`
- Copy: `main/bsp_qmi8658.c` → `components/gamelib_port_esp32s3/bsp_qmi8658.c`

- [ ] **Step 1: 复制 bsp_qmi8658.h (不变)**

`components/gamelib_port_esp32s3/bsp_qmi8658.h` — 与 `main/bsp_qmi8658.h` 完全相同，无修改。

- [ ] **Step 2: 复制并修改 bsp_qmi8658.c**

将 `main/bsp_qmi8658.c` 复制到 `components/gamelib_port_esp32s3/bsp_qmi8658.c`，修改三处：

替换头部硬编码宏:
```c
// 删除:
#define EXAMPLE_I2C_NUM 0 // I2C number
#define EXAMPLE_PIN_NUM_I2C_SDA 48
#define EXAMPLE_PIN_NUM_I2C_SCL 47

// 替换为:
#include "port_config.h"
```

修改 `bsp_i2c_reg8_read` / `bsp_i2c_reg8_write` / `bsp_qmi8658_init` 中所有 `EXAMPLE_I2C_NUM` → `PORT_I2C_NUM`，`EXAMPLE_PIN_NUM_I2C_SDA` → `PORT_I2C_PIN_SDA`，`EXAMPLE_PIN_NUM_I2C_SCL` → `PORT_I2C_PIN_SCL`。

- [ ] **Step 3: 从 main/ 移除旧的 bsp_qmi8658 文件**

删除 `main/bsp_qmi8658.c` 和 `main/bsp_qmi8658.h`（已迁移到 component 中）。

- [ ] **Step 4: Commit**

```bash
git add components/gamelib_port_esp32s3/bsp_qmi8658.c components/gamelib_port_esp32s3/bsp_qmi8658.h
git rm main/bsp_qmi8658.c main/bsp_qmi8658.h
git commit -m "refactor: move bsp_qmi8658 into gamelib_port_esp32s3 component"
```

---

### Task C2: 创建 gamelib_port_esp32s3 component 骨架 + port_config.h

**Files:**
- Create: `components/gamelib_port_esp32s3/CMakeLists.txt`
- Create: `components/gamelib_port_esp32s3/port_config.h`

- [ ] **Step 1: 创建 CMakeLists.txt**

`components/gamelib_port_esp32s3/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "port_display.c" "port_input.c" "port_audio.c" "port_timer.c" "bsp_qmi8658.c"
    INCLUDE_DIRS "."
    REQUIRES gamelib_hal driver esp_timer esp_lcd
)
```

- [ ] **Step 2: 创建 port_config.h**

`components/gamelib_port_esp32s3/port_config.h`:
```c
#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/* === Display: ST7789 via SPI2 === */
#define PORT_LCD_SPI_HOST       SPI2_HOST
#define PORT_LCD_PIN_SCLK       39
#define PORT_LCD_PIN_MOSI       38
#define PORT_LCD_PIN_MISO       40
#define PORT_LCD_PIN_DC         42
#define PORT_LCD_PIN_RST        -1
#define PORT_LCD_PIN_CS         45
#define PORT_LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
#define PORT_LCD_CMD_BITS       8
#define PORT_LCD_PARAM_BITS     8
#define PORT_LCD_H_RES          240
#define PORT_LCD_V_RES          320

/* === Backlight === */
#define PORT_LCD_PIN_BK_LIGHT   1
#define PORT_LCD_BL_LEDC_TIMER  LEDC_TIMER_0
#define PORT_LCD_BL_LEDC_MODE   LEDC_LOW_SPEED_MODE
#define PORT_LCD_BL_LEDC_CH     LEDC_CHANNEL_0
#define PORT_LCD_BL_DUTY_RES    LEDC_TIMER_10_BIT
#define PORT_LCD_BL_FREQ_HZ     10000

/* === Input: GPIO Keys === */
#define PORT_KEY_PIN_UP         4
#define PORT_KEY_PIN_DOWN       5
#define PORT_KEY_PIN_LEFT       6
#define PORT_KEY_PIN_RIGHT      7
#define PORT_KEY_PIN_A          8
#define PORT_KEY_PIN_B          9

/* === Input: QMI8658 (I2C) === */
#define PORT_I2C_NUM            I2C_NUM_0
#define PORT_I2C_PIN_SDA        48
#define PORT_I2C_PIN_SCL        47
#define PORT_TILT_THRESHOLD     500    /* raw accel threshold */

/* === Audio: PWM Buzzer === */
#define PORT_BUZZER_GPIO        2
#define PORT_BUZZER_LEDC_TIMER  LEDC_TIMER_1
#define PORT_BUZZER_LEDC_MODE   LEDC_LOW_SPEED_MODE
#define PORT_BUZZER_LEDC_CH     LEDC_CHANNEL_2
#define PORT_BUZZER_DUTY_RES    LEDC_TIMER_10_BIT

#endif
```

- [ ] **Step 3: Commit**

```bash
git add components/gamelib_port_esp32s3/CMakeLists.txt components/gamelib_port_esp32s3/port_config.h
git commit -m "feat: add gamelib_port_esp32s3 component with pin config"
```

---

### Task C2: port_timer.c

**Files:**
- Create: `components/gamelib_port_esp32s3/port_timer.c`

- [ ] **Step 1: 创建 port_timer.c**

`components/gamelib_port_esp32s3/port_timer.c`:
```c
#include "hal_timer.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void port_timer_init(void) { }

uint32_t port_timer_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

uint32_t port_timer_micros(void)
{
    return (uint32_t)esp_timer_get_time();
}

void port_timer_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void port_timer_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_port_esp32s3/port_timer.c
git commit -m "feat: add ESP32-S3 timer port (esp_timer + FreeRTOS)"
```

---

### Task C3: port_display.c — ST7789 SPI 显示

**Files:**
- Create: `components/gamelib_port_esp32s3/port_display.c`

- [ ] **Step 1: 创建 port_display.c**

`components/gamelib_port_esp32s3/port_display.c`:
```c
#include "hal_display.h"
#include "port_config.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

static const char *TAG = "port_display";
static esp_lcd_panel_handle_t panel_handle = NULL;

static void port_backlight_init(void)
{
    gpio_set_direction(PORT_LCD_PIN_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PORT_LCD_PIN_BK_LIGHT, 1);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = PORT_LCD_BL_LEDC_MODE,
        .timer_num = PORT_LCD_BL_LEDC_TIMER,
        .duty_resolution = PORT_LCD_BL_DUTY_RES,
        .freq_hz = PORT_LCD_BL_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = PORT_LCD_BL_LEDC_MODE,
        .channel = PORT_LCD_BL_LEDC_CH,
        .timer_sel = PORT_LCD_BL_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PORT_LCD_PIN_BK_LIGHT,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void port_backlight_set(uint8_t level)
{
    if (level > 100) level = 100;
    uint32_t duty_max = (1 << PORT_LCD_BL_DUTY_RES) - 1;
    uint32_t duty = (level * duty_max) / 100;
    ledc_set_duty(PORT_LCD_BL_LEDC_MODE, PORT_LCD_BL_LEDC_CH, duty);
    ledc_update_duty(PORT_LCD_BL_LEDC_MODE, PORT_LCD_BL_LEDC_CH);
}

int port_display_init(void)
{
    ESP_LOGI(TAG, "SPI BUS init");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PORT_LCD_PIN_SCLK,
        .mosi_io_num = PORT_LCD_PIN_MOSI,
        .miso_io_num = PORT_LCD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PORT_LCD_H_RES * PORT_LCD_V_RES * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(PORT_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PORT_LCD_PIN_DC,
        .cs_gpio_num = PORT_LCD_PIN_CS,
        .pclk_hz = PORT_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = PORT_LCD_CMD_BITS,
        .lcd_param_bits = PORT_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)PORT_LCD_SPI_HOST,
                                             &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PORT_LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    port_backlight_init();
    port_backlight_set(80);
    return 0;
}

void port_display_deinit(void)
{
    if (panel_handle) {
        esp_lcd_panel_disp_on_off(panel_handle, false);
        esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
    }
}

void port_display_flush(const void *fb, int x, int y, int w, int h)
{
    if (panel_handle) {
        esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w, y + h, (const void *)fb);
    }
}

void port_display_wait_vsync(void) { }

void port_display_backlight(uint8_t level)
{
    port_backlight_set(level);
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_port_esp32s3/port_display.c
git commit -m "feat: add ESP32-S3 ST7789 SPI display port"
```

---

### Task C4: port_input.c — GPIO 按键 + QMI8658 倾斜

**Files:**
- Create: `components/gamelib_port_esp32s3/port_input.c`

- [ ] **Step 1: 创建 port_input.c**

`components/gamelib_port_esp32s3/port_input.c`:
```c
#include "hal_input.h"
#include "port_config.h"
#include "driver/gpio.h"
#include "bsp_qmi8658.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "port_input";

static struct {
    hal_key_t key;
    int      gpio;
    bool     active_low;
} key_map[] = {
    { KEY_UP,    PORT_KEY_PIN_UP,    true },
    { KEY_DOWN,  PORT_KEY_PIN_DOWN,  true },
    { KEY_LEFT,  PORT_KEY_PIN_LEFT,  true },
    { KEY_RIGHT, PORT_KEY_PIN_RIGHT, true },
    { KEY_A,     PORT_KEY_PIN_A,     true },
    { KEY_B,     PORT_KEY_PIN_B,     true },
};

static bool key_state[KEY_COUNT];
static bool key_prev[KEY_COUNT];
static int mouse_x = 0, mouse_y = 0;
static bool mouse_pressed = false;

void port_input_init(void)
{
    memset(key_state, 0, sizeof(key_state));
    memset(key_prev, 0, sizeof(key_prev));

    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        gpio_set_direction(key_map[i].gpio, GPIO_MODE_INPUT);
        gpio_set_pull_mode(key_map[i].gpio, key_map[i].active_low ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
    }
}

void port_input_poll(void)
{
    memcpy(key_prev, key_state, sizeof(key_prev));

    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        int level = gpio_get_level(key_map[i].gpio);
        key_state[key_map[i].key] = key_map[i].active_low ? (level == 0) : (level == 1);
    }

    /* QMI8658 tilt mapping */
    qmi8658_data_t data;
    bsp_qmi8658_read_data(&data);

    bool tilt_left  = (data.acc_x < -PORT_TILT_THRESHOLD);
    bool tilt_right = (data.acc_x >  PORT_TILT_THRESHOLD);
    bool tilt_up    = (data.acc_y < -PORT_TILT_THRESHOLD);
    bool tilt_down  = (data.acc_y >  PORT_TILT_THRESHOLD);

    key_state[KEY_LEFT]  |= tilt_left;
    key_state[KEY_RIGHT] |= tilt_right;
    key_state[KEY_UP]    |= tilt_up;
    key_state[KEY_DOWN]  |= tilt_down;

    /* map tilt to mouse coords: center when flat, offset to edges when tilted */
    mouse_x = 120 + (data.acc_x * 120) / 16000;
    mouse_y = 160 + (data.acc_y * 160) / 16000;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x > 239) mouse_x = 239;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y > 319) mouse_y = 319;

    mouse_pressed = (abs(data.acc_z) > 16000);
}

bool port_input_is_down(hal_key_t key)     { return key_state[key]; }
bool port_input_is_pressed(hal_key_t key)  { return key_state[key] && !key_prev[key]; }
bool port_input_is_released(hal_key_t key) { return !key_state[key] && key_prev[key]; }
int  port_input_mouse_x(void)              { return mouse_x; }
int  port_input_mouse_y(void)              { return mouse_y; }
bool port_input_mouse_down(int button)     { (void)button; return mouse_pressed; }
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_port_esp32s3/port_input.c
git commit -m "feat: add ESP32-S3 GPIO key + QMI8658 tilt input port"
```

---

### Task C5: port_audio.c — PWM 蜂鸣器

**Files:**
- Create: `components/gamelib_port_esp32s3/port_audio.c`

- [ ] **Step 1: 创建 port_audio.c**

`components/gamelib_port_esp32s3/port_audio.c`:
```c
#include "hal_audio.h"
#include "port_config.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "port_audio";
static esp_timer_handle_t beep_timer = NULL;
static bool beeping = false;

static void beep_timer_cb(void *arg)
{
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = false;
}

int port_audio_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = PORT_BUZZER_LEDC_MODE,
        .timer_num = PORT_BUZZER_LEDC_TIMER,
        .duty_resolution = PORT_BUZZER_DUTY_RES,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = PORT_BUZZER_LEDC_MODE,
        .channel = PORT_BUZZER_LEDC_CH,
        .timer_sel = PORT_BUZZER_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PORT_BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    const esp_timer_create_args_t timer_args = {
        .callback = beep_timer_cb,
        .name = "beep_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &beep_timer));
    return 44100;  /* pretend sample rate */
}

void port_audio_deinit(void)
{
    if (beep_timer) {
        esp_timer_stop(beep_timer);
        esp_timer_delete(beep_timer);
        beep_timer = NULL;
    }
}

void port_audio_beep(int freq_hz, int duration_ms)
{
    if (freq_hz <= 0) return;

    ledc_set_freq(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_TIMER, (uint32_t)freq_hz);
    uint32_t duty_max = (1 << PORT_BUZZER_DUTY_RES) - 1;
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, duty_max / 2);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = true;

    if (duration_ms > 0 && beep_timer) {
        esp_timer_stop(beep_timer);
        ESP_ERROR_CHECK(esp_timer_start_once(beep_timer, duration_ms * 1000));
    }
}

void port_audio_stop(void)
{
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = false;
    if (beep_timer) esp_timer_stop(beep_timer);
}

bool port_audio_is_busy(void)
{
    return beeping;
}
```

- [ ] **Step 2: Commit**

```bash
git add components/gamelib_port_esp32s3/port_audio.c
git commit -m "feat: add ESP32-S3 PWM buzzer audio port"
```

---

## Phase D: 集成与验证

### Task D1: 注册 port 函数到 HAL 全局实例

**Files:**
- Create: `components/gamelib_port_esp32s3/port_hal_init.c`

- [ ] **Step 1: 创建 HAL 注册文件**

`components/gamelib_port_esp32s3/port_hal_init.c`:
```c
#include "gamelib.h"
#include "port_config.h"

/* port 函数声明 */
int  port_display_init(void);
void port_display_deinit(void);
void port_display_flush(const void *fb, int x, int y, int w, int h);
void port_display_wait_vsync(void);
void port_display_backlight(uint8_t level);

void port_input_init(void);
void port_input_poll(void);
bool port_input_is_down(hal_key_t key);
bool port_input_is_pressed(hal_key_t key);
bool port_input_is_released(hal_key_t key);
int  port_input_mouse_x(void);
int  port_input_mouse_y(void);
bool port_input_mouse_down(int button);

int  port_audio_init(void);
void port_audio_deinit(void);
void port_audio_beep(int freq_hz, int duration_ms);
void port_audio_stop(void);
bool port_audio_is_busy(void);

void port_timer_init(void);
uint32_t port_timer_millis(void);
uint32_t port_timer_micros(void);
void port_timer_delay_ms(uint32_t ms);
void port_timer_sleep_ms(uint32_t ms);

/* 注册到全局 g_hal */
void gamelib_port_register_hal(void)
{
    g_hal.display.width  = PORT_LCD_H_RES;
    g_hal.display.height = PORT_LCD_V_RES;
    g_hal.display.bpp    = 16;
    g_hal.display.init       = port_display_init;
    g_hal.display.deinit     = port_display_deinit;
    g_hal.display.flush      = port_display_flush;
    g_hal.display.wait_vsync = port_display_wait_vsync;
    g_hal.display.backlight  = port_display_backlight;

    g_hal.input.init       = port_input_init;
    g_hal.input.poll       = port_input_poll;
    g_hal.input.is_down    = port_input_is_down;
    g_hal.input.is_pressed = port_input_is_pressed;
    g_hal.input.is_released = port_input_is_released;
    g_hal.input.mouse_x    = port_input_mouse_x;
    g_hal.input.mouse_y    = port_input_mouse_y;
    g_hal.input.mouse_down = port_input_mouse_down;

    g_hal.audio.init   = port_audio_init;
    g_hal.audio.deinit = port_audio_deinit;
    g_hal.audio.beep   = port_audio_beep;
    g_hal.audio.stop   = port_audio_stop;
    g_hal.audio.is_busy = port_audio_is_busy;

    g_hal.timer.init     = port_timer_init;
    g_hal.timer.millis   = port_timer_millis;
    g_hal.timer.micros   = port_timer_micros;
    g_hal.timer.delay_ms = port_timer_delay_ms;
    g_hal.timer.sleep_ms = port_timer_sleep_ms;
}
```

- [ ] **Step 2: 更新 CMakeLists.txt 加入新文件**

`components/gamelib_port_esp32s3/CMakeLists.txt` 更新为:
```cmake
idf_component_register(
    SRCS "port_display.c" "port_input.c" "port_audio.c" "port_timer.c" "port_hal_init.c"
    INCLUDE_DIRS "."
    REQUIRES gamelib_hal gamelib_core driver esp_timer esp_lcd
)
```

- [ ] **Step 3: Commit**

```bash
git add components/gamelib_port_esp32s3/port_hal_init.c components/gamelib_port_esp32s3/CMakeLists.txt
git commit -m "feat: add HAL registration glue"
```

---

### Task D2: 更新 main/CMakeLists.txt + 主应用代码

**Files:**
- Modify: `main/CMakeLists.txt`
- Modify: `main/main.c`

- [ ] **Step 1: 更新 main/CMakeLists.txt**

`main/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES gamelib_core gamelib_port_esp32s3)
```

- [ ] **Step 2: 创建 main.c MVE 示例**

`main/main.c`:
```c
#include <stdio.h>
#include "gamelib.h"

/* 外部声明：HAL 注册函数 */
void gamelib_port_register_hal(void);

/* 嵌入 BMP（用 xxd -i 转换后 include） */
/* 示例：画一个程序化的精灵用于演示 */
#define PLAYER_W 16
#define PLAYER_H 16
static const uint8_t player_bmp[] = {
    /* 一个简单的 16x16 笑脸 BMP 数据可嵌入在此，或使用 xxd -i 转换 */
    /* 此处使用程序化精灵代替 BMP 文件 */
};

gamelib_t game;

void app_main(void)
{
    /* 1. 注册 HAL */
    gamelib_port_register_hal();

    /* 2. 初始化 GameLib */
    if (gamelib_init(&game, 240, 320, 60) != 0) {
        printf("GameLib init failed\n");
        return;
    }

    /* 3. 创建精灵 */
    int sprite_id = gamelib_sprite_create(&game, 16, 16);
    if (sprite_id >= 0) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                int dx = x - 8, dy = y - 8;
                gamelib_color_t c = (dx * dx + dy * dy < 49) ? COLOR_RED : COLOR_NONE;
                gamelib_sprite_set_pixel(&game, sprite_id, x, y, c);
            }
        }
    }

    int player_x = 112, player_y = 152;  /* center of 240x320 */

    /* 4. 游戏主循环 */
    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_DARKGRAY);

        /* 绘制文字 */
        gamelib_draw_text(&game, 5, 5, "GameLib ESP32 MVE", COLOR_WHITE);
        gamelib_draw_text(&game, 5, 20, "Hello World!", COLOR_YELLOW);
        gamelib_draw_number(&game, 5, 300, (int)gamelib_get_fps(&game), COLOR_GREEN);
        gamelib_draw_text(&game, 40, 300, "FPS", COLOR_GREEN);

        /* 绘制图形 */
        gamelib_draw_rect(&game, 10, 40, 50, 30, COLOR_CYAN);
        gamelib_fill_circle(&game, 200, 55, 15, COLOR_MAGENTA);
        gamelib_draw_line(&game, 10, 80, 80, 120, COLOR_YELLOW);

        /* 按键控制 */
        if (gamelib_is_key_down(&game, KEY_LEFT))  player_x -= 2;
        if (gamelib_is_key_down(&game, KEY_RIGHT)) player_x += 2;
        if (gamelib_is_key_down(&game, KEY_UP))    player_y -= 2;
        if (gamelib_is_key_down(&game, KEY_DOWN))  player_y += 2;

        /* 按 A 键蜂鸣 */
        if (gamelib_is_key_pressed(&game, KEY_A)) {
            gamelib_play_beep(&game, 880, 100);
        }

        /* 边界 */
        if (player_x < 0) player_x = 0;
        if (player_x > 224) player_x = 224;
        if (player_y < 0) player_y = 0;
        if (player_y > 304) player_y = 304;

        /* 绘制精灵 */
        gamelib_draw_sprite(&game, sprite_id, player_x, player_y);

        /* 帧结束 */
        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }

    gamelib_deinit(&game);
}
```

- [ ] **Step 3: Commit**

```bash
git add main/CMakeLists.txt main/main.c
git commit -m "feat: add MVE demo application"
```

---

### Task D3: 编译并验证

- [ ] **Step 1: 设置目标芯片**

```bash
cd gamelib_for_esp32
idf.py set-target esp32s3
```

预期：目标芯片已设为 esp32s3，sdkconfig 更新。

- [ ] **Step 2: 编译项目**

```bash
idf.py build
```

预期：编译通过无错误。

- [ ] **Step 3: 烧录并监控**

```bash
idf.py flash monitor
```

预期：屏幕显示 GameLib MVE 画面（文字 + 红色圆形精灵 + 图形），按键可移动精灵，按 A 键蜂鸣。

- [ ] **Step 4: Commit**

```bash
git add -p
git commit -m "build: verify MVE compiles and runs on ESP32-S3"
```

---

## 验证清单

编译成功后，在 ESP32-S3 开发板上确认以下行为：

- [ ] 屏幕显示 "GameLib ESP32 MVE" 和 "Hello World!" 文字
- [ ] 红色圆形精灵（16x16）在屏幕中央可见
- [ ] 青色矩形、洋红色圆、黄色线段可见
- [ ] FPS 数字在屏幕底部变化
- [ ] GPIO 按键可移动精灵（上下左右）
- [ ] 按 A 键时蜂鸣器短鸣
- [ ] 倾斜开发板可移动精灵（QMI8658 倾斜控制）
- [ ] 倾斜开发板时光标位置跟随变化
