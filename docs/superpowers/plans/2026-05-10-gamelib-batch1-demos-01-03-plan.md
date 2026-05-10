# GameLib ESP32-S3 Batch1: Demo 01-03 Support

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增 GetTime/GetDeltaTime/GetWidth/GetHeight/DrawPrintf API 及颜色宏，支持 demo 01-03 全部功能，不破坏 MVE。

**Architecture:** 本次改动仅涉及 3 个现有文件，均为追加式修改。hal_types.h 加颜色宏，gamelib.h 加 struct 字段和 API 声明，gamelib.c 加实现。无新文件、无向后不兼容变更。

**Tech Stack:** C11, ESP-IDF, RGB565 字节交换格式

---

### Task 1: 补充预定义颜色 + COLOR_RGB 宏

**Files:**
- Modify: `components/gamelib_hal/hal_types.h`

- [ ] **Step 1: 在 COLOR_LIGHTGRAY 之后插入新颜色定义**

在 `#define COLOR_LIGHTGRAY 0x18C6` 之后，`#define COLOR_WHITE 0xFFFF` 之前，插入以下内容：

```c
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
#define COLOR_DARK_RED    0x0088
```

- [ ] **Step 2: 在颜色定义块末尾（COLOR_WHITE 之后）添加 COLOR_RGB 宏**

```c
/* Compose color from 8-bit RGB channels (byte-swapped for SPI DMA) */
#define COLOR_RGB(r, g, b) \
    ((uint16_t)(((r) & 0xF8) | ((g) >> 5)) | \
     ((uint16_t)(((g) & 0x1C) << 3) | ((b) >> 3)) << 8)
```

- [ ] **Step 3: 验证颜色值**

Run: `grep -E "COLOR_(NAVY|DARK_GREEN|DARK_CYAN|MAROON|PURPLE|OLIVE|ORANGE|GREEN_YELLOW|PINK|BROWN|GOLD|DARK_BLUE|DARK_RED)" components/gamelib_hal/hal_types.h`

Expected: 13 个新颜色 define 全部存在，值如 spec 所定义。

- [ ] **Step 4: 编译验证**

Run: `idf.py build`

Expected: 编译通过，无错误。

- [ ] **Step 5: Commit**

```bash
git add components/gamelib_hal/hal_types.h
git commit -m "feat: add 13 extended colors and COLOR_RGB macro (byte-swapped RGB565)"
```

---

### Task 2: gamelib.h — 新增字段与 API 声明

**Files:**
- Modify: `components/gamelib_core/gamelib.h`

- [ ] **Step 1: 在 gamelib_t 结构体中新增 start_time 字段**

在 `double frame_start;` 之后、`int target_fps;` 之前，插入：

```c
    double        start_time;    /* init 时刻时间戳(秒) */
```

最终结构体顺序为：

```c
    double        delta_time;
    double        fps;
    double        frame_start;
    double        start_time;    /* init 时刻时间戳(秒) */
    int           target_fps;
```

- [ ] **Step 2: 在现有 API 声明区域添加新函数声明**

在当前 `gamelib_get_fps` 声明之后、`/* --- drawing --- */` 注释之前，插入：

```c
/* --- time --- */
double gamelib_get_time(gamelib_t *g);
double gamelib_get_delta_time(gamelib_t *g);
int    gamelib_get_width(gamelib_t *g);
int    gamelib_get_height(gamelib_t *g);
```

- [ ] **Step 3: 在文字 API 区域添加 gamelib_draw_printf 声明**

在当前 `gamelib_draw_number` 声明之后、`/* --- sprite --- */` 注释之前，插入：

```c
void gamelib_draw_printf(gamelib_t *g, int x, int y, gamelib_color_t c, const char *fmt, ...);
```

- [ ] **Step 4: 编译验证**

Run: `idf.py build`

Expected: 编译通过（可能有 link 错误因为 gamelib.c 尚未实现新函数，若 ESP-IDF 允许未定义引用则通过，否则见 Task 3）。

- [ ] **Step 5: Commit**

```bash
git add components/gamelib_core/gamelib.h
git commit -m "feat: add start_time field and declare new time/printf APIs"
```

---

### Task 3: gamelib.c — 实现新增 API

**Files:**
- Modify: `components/gamelib_core/gamelib.c`

- [ ] **Step 1: 添加必要的头文件**

在现有 `#include <math.h>` 之后添加：

```c
#include <stdio.h>
#include <stdarg.h>
```

- [ ] **Step 2: 在 gamelib_init 中初始化 start_time**

在 `gamelib_init()` 函数中，`g->running = true;` 之前，添加：

```c
    g->start_time = (double)g_hal.timer.micros() / 1000000.0;
```

放在 `g->frame_start = ...` 之后、`g->running = true;` 之前。最终该区域代码如下：

```c
    g->frame_start = (double)g_hal.timer.micros() / 1000000.0;
    g->start_time  = g->frame_start;
    g->running = true;
    return 0;
```

- [ ] **Step 3: 在 gamelib_get_fps 之后实现 gamelib_get_time**

在 `gamelib_get_fps()` 函数定义之后，插入：

```c
double gamelib_get_time(gamelib_t *g)
{
    double now = (double)g_hal.timer.micros() / 1000000.0;
    return now - g->start_time;
}
```

- [ ] **Step 4: 实现 gamelib_get_delta_time**

紧接其后：

```c
double gamelib_get_delta_time(gamelib_t *g)
{
    return g->delta_time;
}
```

- [ ] **Step 5: 实现 gamelib_get_width 和 gamelib_get_height**

紧接其后：

```c
int gamelib_get_width(gamelib_t *g)
{
    return g->fb.width;
}

int gamelib_get_height(gamelib_t *g)
{
    return g->fb.height;
}
```

- [ ] **Step 6: 在 gamelib_mouse_y 之后实现 gamelib_draw_printf**

在 `gamelib_mouse_y()` 函数定义之后、`/* --- helpers --- */` 注释之前，插入：

```c
void gamelib_draw_printf(gamelib_t *g, int x, int y, gamelib_color_t c, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gamelib_draw_text(g, x, y, buf, c);
}
```

- [ ] **Step 7: 编译验证**

Run: `idf.py build`

Expected: 编译通过，无错误，无警告。

- [ ] **Step 8: Commit**

```bash
git add components/gamelib_core/gamelib.c
git commit -m "feat: implement GetTime, GetDeltaTime, GetWidth, GetHeight, DrawPrintf"
```

---

### Task 4: 更新 main.c MVE 验证新 API

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: 替换 main.c 为验证新增 API 的代码**

将 `main/app_main` 中的主循环部分改为同时使用新旧 API，验证所有新增功能正常工作：

```c
#include <stdio.h>
#include "gamelib.h"

/* HAL registration */
void gamelib_port_register_hal(void);

static gamelib_t game;

void app_main(void)
{
    gamelib_port_register_hal();

    if (gamelib_init(&game, 240, 320, 60) != 0) {
        printf("GameLib init failed\n");
        return;
    }

    /* create a procedural sprite (red circle) */
    int sprite_id = gamelib_sprite_create(&game, 16, 16);
    if (sprite_id >= 0) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                int dx = x - 8, dy = y - 8;
                gamelib_color_t c = (dx * dx + dy * dy < 49) ? COLOR_RED : COLOR_NONE;
                gamelib_sprite_set_pixel(&game, sprite_id, x, y, c);
            }
        }
        gamelib_sprite_set_color_key(&game, sprite_id, COLOR_NONE);
    }

    int player_x = 112, player_y = 152;

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_DARKGRAY);

        /* New APIs: GetWidth/GetHeight for boundary calc */
        int w = gamelib_get_width(&game);
        int h = gamelib_get_height(&game);

        /* New API: DrawPrintf */
        gamelib_draw_printf(&game, 5, 5, COLOR_WHITE, "GameLib ESP32 MVE v2");
        gamelib_draw_text(&game, 5, 20, "Hello World!", COLOR_YELLOW);

        /* New API: GetTime + GetDeltaTime */
        gamelib_draw_printf(&game, 5, h - 35, COLOR_GREEN, "FPS: %.0f", gamelib_get_fps(&game));
        gamelib_draw_printf(&game, 5, h - 20, COLOR_CYAN, "Time: %.1fs  dt: %.3fs",
                            gamelib_get_time(&game), gamelib_get_delta_time(&game));

        /* Original drawing */
        gamelib_draw_rect(&game, 10, 40, 50, 30, COLOR_CYAN);
        gamelib_fill_circle(&game, 200, 55, 15, COLOR_MAGENTA);
        gamelib_draw_line(&game, 10, 80, 80, 120, COLOR_YELLOW);

        /* New color test: COLOR_RGB macro */
        gamelib_fill_rect(&game, 10, 140, 30, 30, COLOR_RGB(255, 165, 0));   // Orange
        gamelib_fill_rect(&game, 45, 140, 30, 30, COLOR_RGB(255, 215, 0));   // Gold
        gamelib_fill_rect(&game, 80, 140, 30, 30, COLOR_ORANGE);             // predefined
        gamelib_fill_rect(&game, 115, 140, 30, 30, COLOR_GOLD);              // predefined
        gamelib_draw_text(&game, 10, 175, "COLOR_RGB + extended palette", COLOR_LIGHTGRAY);

        if (gamelib_is_key_down(&game, KEY_LEFT))  player_x -= 2;
        if (gamelib_is_key_down(&game, KEY_RIGHT)) player_x += 2;
        if (gamelib_is_key_down(&game, KEY_UP))    player_y -= 2;
        if (gamelib_is_key_down(&game, KEY_DOWN))  player_y += 2;

        if (gamelib_is_key_pressed(&game, KEY_A)) {
            gamelib_play_beep(&game, 880, 100);
            gamelib_draw_printf(&game, 5, h - 50, COLOR_YELLOW, "Beep!");
        }

        /* boundary check uses new GetWidth/GetHeight */
        if (player_x < 0) player_x = 0;
        if (player_x > w - 16) player_x = w - 16;
        if (player_y < 0) player_y = 0;
        if (player_y > h - 16) player_y = h - 16;

        gamelib_draw_sprite(&game, sprite_id, player_x, player_y);

        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }

    gamelib_deinit(&game);
}
```

- [ ] **Step 2: 编译验证**

Run: `idf.py build`

Expected: 编译通过，无错误。

- [ ] **Step 3: Commit**

```bash
git add main/main.c
git commit -m "feat: update MVE to exercise new time, printf and color APIs"
```

---

### Task 5: 全量编译 + 运行验证

**Files:** 无新建

- [ ] **Step 1: 完整构建**

Run: `idf.py build`

Expected: 编译 + 链接全部通过。

- [ ] **Step 2: 烧录并观察串口输出**

Run: `idf.py flash monitor`

Expected:
- 屏幕显示 "GameLib ESP32 MVE v2" 标题
- 底部两行分别显示 FPS 和 Time/dt 信息
- 4 个色块（Orange + Gold，通过 COLOR_RGB 宏和预定义色各 2 个）并排显示在 (10,140) 行
- 方向键移动红色圆形精灵，边界碰撞生效
- 按 A 键蜂鸣器响，屏幕短暂显示 "Beep!"

- [ ] **Step 3: Commit (如有微调)**

如有微调则提交，否则跳过。

---

### 变更总结

| 文件 | 行数变化 | 变更类型 |
|------|---------|---------|
| `hal_types.h` | +15 | 13 新颜色 + COLOR_RGB 宏 |
| `gamelib.h` | +6 | start_time 字段 + 5 个 API 声明 |
| `gamelib.c` | +35 | 5 个新函数实现 + 初始化 start_time |
| `main.c` | 重写 | 演示所有新增 API |
