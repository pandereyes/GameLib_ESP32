# GameLib ESP32-S3 Batch2: Sprite Flip/Scale/Frame Animation

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 新增 draw_sprite_ex / draw_sprite_scaled / draw_sprite_frame_scaled，支持 demo 05 精灵翻转/缩放/帧动画。

**Architecture:** 所有实现在 sprite.c 内部，复用现有精灵数据结构。draw_sprite_frame_scaled 将帧提取 + 缩放 + 翻转合并为一次遍历，避免中间缓冲。翻转用索引映射（无额外内存），缩放用最近邻整数除法。

**Tech Stack:** C11, ESP-IDF, RGB565 framebuffer

**Constraint:** 不破坏现有 API，不自动 commit。

---

### Task 1: gamelib.h — 新增翻转宏和 API 声明

**Files:**
- Modify: `components/gamelib_core/gamelib.h`

- [ ] **Step 1: 在 sprite 相关代码区域之前添加翻转标志宏**

在 `/* --- sprite --- */` 注释行之前，插入：

```c
/* --- sprite flip flags --- */
#define SPRITE_FLIP_H  1
#define SPRITE_FLIP_V  2
```

- [ ] **Step 2: 在现有 gamelib_draw_sprite_region 声明之后添加 3 个新 API 声明**

在 `gamelib_draw_sprite_region` 声明行之后、`gamelib_sprite_width` 声明之前，插入：

```c
void gamelib_draw_sprite_ex(gamelib_t *g, int id, int x, int y, int flags);
void gamelib_draw_sprite_scaled(gamelib_t *g, int id, int x, int y, int dst_w, int dst_h);
void gamelib_draw_sprite_frame_scaled(gamelib_t *g, int id, int x, int y,
                                       int fw, int fh, int frame, int dst_w, int dst_h,
                                       int flags);
```

- [ ] **Step 3: 编译验证**

Run: `idf.py build`

Expected: 编译通过（可能有 link 警告，取决于是否先实现）

---

### Task 2: sprite.c — 实现三个新函数

**Files:**
- Modify: `components/gamelib_core/sprite.c`

- [ ] **Step 1: 在 gamelib_draw_sprite 函数之后实现 gamelib_draw_sprite_ex**

```c
void gamelib_draw_sprite_ex(gamelib_t *g, int id, int dst_x, int dst_y, int flags)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    int sw = sp->width;
    int sh = sp->height;
    framebuffer_t *fb = &g->fb;

    for (int row = 0; row < sh; row++) {
        for (int col = 0; col < sw; col++) {
            int src_x = (flags & SPRITE_FLIP_H) ? (sw - 1 - col) : col;
            int src_y = (flags & SPRITE_FLIP_V) ? (sh - 1 - row) : row;
            gamelib_color_t c = sp->pixels[src_y * sw + src_x];
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

- [ ] **Step 2: 实现 gamelib_draw_sprite_scaled（最近邻）**

```c
void gamelib_draw_sprite_scaled(gamelib_t *g, int id, int dst_x, int dst_y, int dst_w, int dst_h)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    int sw = sp->width;
    int sh = sp->height;
    if (dst_w <= 0 || dst_h <= 0) return;
    framebuffer_t *fb = &g->fb;

    for (int row = 0; row < dst_h; row++) {
        int src_y = row * sh / dst_h;
        for (int col = 0; col < dst_w; col++) {
            int src_x = col * sw / dst_w;
            gamelib_color_t c = sp->pixels[src_y * sw + src_x];
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

- [ ] **Step 3: 实现 gamelib_draw_sprite_frame_scaled**

```c
void gamelib_draw_sprite_frame_scaled(gamelib_t *g, int id, int dst_x, int dst_y,
                                       int fw, int fh, int frame, int dst_w, int dst_h,
                                       int flags)
{
    if (id < 0 || id >= MAX_SPRITES || !g->sprites[id].used) return;
    sprite_t *sp = &g->sprites[id];
    if (dst_w <= 0 || dst_h <= 0 || fw <= 0 || fh <= 0) return;

    int sheet_w = sp->width;
    int sheet_h = sp->height;
    int src_ox = frame * fw;  /* frame origin X in sheet */
    if (src_ox + fw > sheet_w || fh > sheet_h) return;  /* frame out of bounds */
    framebuffer_t *fb = &g->fb;

    for (int row = 0; row < dst_h; row++) {
        int fy = row * fh / dst_h;  /* pixel within frame */
        int src_y = (flags & SPRITE_FLIP_V) ? (fh - 1 - fy) : fy;
        for (int col = 0; col < dst_w; col++) {
            int fx = col * fw / dst_w;
            int src_x = src_ox + ((flags & SPRITE_FLIP_H) ? (fw - 1 - fx) : fx);
            gamelib_color_t c = sp->pixels[src_y * sheet_w + src_x];
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

- [ ] **Step 4: 编译验证**

Run: `idf.py build`

Expected: 编译 + 链接通过，无错误无警告。

---

### Task 3: main.c — 演示精灵翻转和缩放

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: 新增 sprite 演示（保持原有 MVE 功能，在前半段绘制后追加 sprite demo）**

替换 main.c 为以下内容（保留所有原有绘制和输入逻辑，在右上角新增精灵翻转/缩放演示区）：

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

    /* --- Batch1: procedural sprite (red circle) --- */
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

    /* --- Batch2: 8x8 4-frame anim sheet (pulsing circle) --- */
    int anim_id = gamelib_sprite_create(&game, 32, 8);
    if (anim_id >= 0) {
        gamelib_color_t colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_WHITE};
        int sizes[] = {1, 2, 3, 2};
        for (int f = 0; f < 4; f++) {
            int cx = f * 8 + 4, cy = 4;
            for (int dy = -sizes[f]; dy <= sizes[f]; dy++)
                for (int dx = -sizes[f]; dx <= sizes[f]; dx++)
                    if (dx * dx + dy * dy <= sizes[f] * sizes[f])
                        gamelib_sprite_set_pixel(&game, anim_id, cx + dx, cy + dy, colors[f]);
        }
        gamelib_sprite_set_color_key(&game, anim_id, COLOR_NONE);
    }

    int player_x = 112, player_y = 152;
    int frame = 0, frame_timer = 0;

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_DARKGRAY);

        int w = gamelib_get_width(&game);
        int h = gamelib_get_height(&game);

        /* HUD */
        gamelib_draw_printf(&game, 5, 5, COLOR_WHITE, "GameLib ESP32 MVE v3");
        gamelib_draw_printf(&game, 5, h - 35, COLOR_GREEN, "FPS: %.0f", gamelib_get_fps(&game));
        gamelib_draw_printf(&game, 5, h - 20, COLOR_CYAN, "Time: %.1fs", gamelib_get_time(&game));

        /* Original drawing */
        gamelib_draw_rect(&game, 10, 40, 50, 30, COLOR_CYAN);
        gamelib_fill_circle(&game, 200, 55, 15, COLOR_MAGENTA);
        gamelib_draw_line(&game, 10, 80, 80, 120, COLOR_YELLOW);

        /* Color test */
        gamelib_fill_rect(&game, 10, 140, 20, 20, COLOR_RGB(255, 165, 0));
        gamelib_fill_rect(&game, 35, 140, 20, 20, COLOR_GOLD);
        gamelib_draw_text(&game, 65, 145, "Batch1 colors", COLOR_LIGHTGRAY);

        /* --- Batch2: sprite flip demo --- */
        gamelib_draw_text(&game, 10, 175, "Flip:", COLOR_WHITE);
        gamelib_draw_sprite(&game, sprite_id, 10, 190);          /* normal */
        gamelib_draw_sprite_ex(&game, sprite_id, 35, 190, SPRITE_FLIP_H);
        gamelib_draw_sprite_ex(&game, sprite_id, 60, 190, SPRITE_FLIP_V);
        gamelib_draw_sprite_ex(&game, sprite_id, 85, 190, SPRITE_FLIP_H | SPRITE_FLIP_V);

        /* --- Batch2: sprite scaled demo --- */
        gamelib_draw_text(&game, 10, 215, "Scale:", COLOR_WHITE);
        gamelib_draw_sprite_scaled(&game, sprite_id, 10, 230, 32, 32);   /* 2x */
        gamelib_draw_sprite_scaled(&game, sprite_id, 50, 230, 48, 48);   /* 3x */

        /* --- Batch2: frame anim demo --- */
        frame_timer++;
        if (frame_timer >= 10) { frame_timer = 0; frame = (frame + 1) % 4; }
        gamelib_draw_text(&game, 120, 175, "Frame Anim:", COLOR_WHITE);
        gamelib_draw_sprite_scaled(&game, anim_id, 120, 190, 64, 16);  /* full sheet */
        gamelib_draw_sprite_frame_scaled(&game, anim_id, 120, 215, 8, 8, frame, 32, 32, 0);
        gamelib_draw_sprite_frame_scaled(&game, anim_id, 160, 215, 8, 8, frame, 32, 32, SPRITE_FLIP_H);

        /* Original input + movement */
        if (gamelib_is_key_down(&game, KEY_LEFT))  player_x -= 2;
        if (gamelib_is_key_down(&game, KEY_RIGHT)) player_x += 2;
        if (gamelib_is_key_down(&game, KEY_UP))    player_y -= 2;
        if (gamelib_is_key_down(&game, KEY_DOWN))  player_y += 2;

        if (gamelib_is_key_pressed(&game, KEY_A)) {
            gamelib_play_beep(&game, 880, 100);
            gamelib_draw_printf(&game, 5, h - 50, COLOR_YELLOW, "Beep! A pressed");
        }

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

---

### 变更总结

| 文件 | 变更 | 行数 |
|------|------|------|
| `gamelib.h` | +3 声明 + 2 宏 | +6 |
| `sprite.c` | +3 函数实现 | +70 |
| `main.c` | 新增精灵翻转/缩放/帧动画演示 | 重写 |
