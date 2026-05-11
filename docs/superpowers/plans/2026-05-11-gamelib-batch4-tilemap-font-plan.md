# GameLib ESP32-S3 Batch4: Tilemap + TTF Font + Grid

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. No auto-commit.

**Goal:** 新增 Tilemap 子系统、TTF 字体渲染（stb_truetype）、网格绘图、SPRITE_COLORKEY 支持。

**Architecture:** tilemap.c 独立子系统（tiles 数组 PSRAM，渲染走 draw_sprite_region）；font.c 管理 stb_truetype glyph 缓存；网格绘图在 gamelib.c 中实现。

**Tech Stack:** C11, stb_truetype.h

---

### Task 1: 新建 tilemap.h + tilemap.c

**Files:**
- Create: `components/gamelib_core/tilemap.h`
- Create: `components/gamelib_core/tilemap.c`

#### tilemap.h

```c
#ifndef TILEMAP_H
#define TILEMAP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t *tiles;
    int      cols;
    int      rows;
    int      tile_size;
    int      tileset_id;
    bool     used;
} tilemap_t;

#endif
```

#### tilemap.c

```c
#include "tilemap.h"
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>

#define MAX_TILEMAPS 8
extern tilemap_t *g_tilemaps;  /* defined in gamelib.c */
extern int        g_tilemap_count;

static tilemap_t* tilemap_get(int map_id)
{
    if (map_id < 0 || map_id >= MAX_TILEMAPS) return NULL;
    if (!g_tilemaps || !g_tilemaps[map_id].used) return NULL;
    return &g_tilemaps[map_id];
}

int gamelib_tilemap_create(gamelib_t *g, int cols, int rows, int tile_size, int tileset_id)
{
    if (!g_tilemaps) return -1;
    for (int i = 0; i < MAX_TILEMAPS; i++) {
        if (!g_tilemaps[i].used) {
            tilemap_t *tm = &g_tilemaps[i];
            tm->tiles = (int16_t*)calloc(cols * rows, sizeof(int16_t));
            if (!tm->tiles) return -1;
            for (int j = 0; j < cols * rows; j++) tm->tiles[j] = -1;
            tm->cols = cols;
            tm->rows = rows;
            tm->tile_size = tile_size;
            tm->tileset_id = tileset_id;
            tm->used = true;
            return i;
        }
    }
    return -1;
}

void gamelib_tilemap_free(gamelib_t *g, int map_id)
{
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    free(tm->tiles);
    memset(tm, 0, sizeof(*tm));
}

void gamelib_tilemap_clear(gamelib_t *g, int map_id, int tile_id)
{
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    int n = tm->cols * tm->rows;
    for (int i = 0; i < n; i++) tm->tiles[i] = (int16_t)tile_id;
}

void gamelib_tilemap_set_tile(gamelib_t *g, int map_id, int col, int row, int tile_id)
{
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm || col < 0 || col >= tm->cols || row < 0 || row >= tm->rows) return;
    tm->tiles[row * tm->cols + col] = (int16_t)tile_id;
}

void gamelib_tilemap_fill_rect(gamelib_t *g, int map_id, int col, int row, int w, int h, int tile_id)
{
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    for (int r = row; r < row + h && r < tm->rows; r++) {
        for (int c = col; c < col + w && c < tm->cols; c++) {
            if (r >= 0 && c >= 0) tm->tiles[r * tm->cols + c] = (int16_t)tile_id;
        }
    }
}

int gamelib_tilemap_get_cols(gamelib_t *g, int map_id) { tilemap_t *tm = tilemap_get(map_id); return tm ? tm->cols : 0; }
int gamelib_tilemap_get_rows(gamelib_t *g, int map_id) { tilemap_t *tm = tilemap_get(map_id); return tm ? tm->rows : 0; }
int gamelib_tilemap_get_tile_size(gamelib_t *g, int map_id) { tilemap_t *tm = tilemap_get(map_id); return tm ? tm->tile_size : 0; }

void gamelib_tilemap_draw(gamelib_t *g, int map_id, int offset_x, int offset_y)
{
    tilemap_t *tm = tilemap_get(map_id);
    if (!tm) return;
    int ts = tm->tile_size;
    int screen_w = g->fb.width;
    int screen_h = g->fb.height;

    int start_col = (-offset_x) / ts;
    if (start_col < 0) start_col = 0;
    int start_row = (-offset_y) / ts;
    if (start_row < 0) start_row = 0;
    int end_col = (screen_w - offset_x + ts - 1) / ts;
    if (end_col > tm->cols) end_col = tm->cols;
    int end_row = (screen_h - offset_y + ts - 1) / ts;
    if (end_row > tm->rows) end_row = tm->rows;

    for (int row = start_row; row < end_row; row++) {
        for (int col = start_col; col < end_col; col++) {
            int tid = tm->tiles[row * tm->cols + col];
            if (tid < 0) continue;
            int sx = tid * ts;
            int sy = 0;
            /* wrap for multi-row tilesets */
            if (tm->tileset_id >= 0) {
                int sp_w = gamelib_sprite_width(g, tm->tileset_id);
                int cols_in_sheet = sp_w / ts;
                if (cols_in_sheet > 0) {
                    sy = (tid / cols_in_sheet) * ts;
                    sx = (tid % cols_in_sheet) * ts;
                }
            }
            int dx = col * ts + offset_x;
            int dy = row * ts + offset_y;
            gamelib_draw_sprite_region(g, tm->tileset_id, dx, dy, sx, sy, ts, ts);
        }
    }
}

int gamelib_tilemap_world_to_col(gamelib_t *g, int map_id, int world_x) { tilemap_t *tm = tilemap_get(map_id); return tm ? world_x / tm->tile_size : 0; }
int gamelib_tilemap_world_to_row(gamelib_t *g, int map_id, int world_y) { tilemap_t *tm = tilemap_get(map_id); return tm ? world_y / tm->tile_size : 0; }
int gamelib_tilemap_get_at_pixel(gamelib_t *g, int map_id, int world_x, int world_y) { tilemap_t *tm = tilemap_get(map_id); if (!tm) return -1; int c = world_x / tm->tile_size; int r = world_y / tm->tile_size; if (c < 0 || c >= tm->cols || r < 0 || r >= tm->rows) return -1; return tm->tiles[r * tm->cols + c]; }
```

---

### Task 2: 新建 font.h + font.c + stb_truetype.h

**Files:**
- Create: `components/gamelib_core/font.h`
- Create: `components/gamelib_core/font.c`
- Create: `components/gamelib_core/stb_truetype.h` (download from https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h)

#### font.h

```c
#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define MAX_FONTS 4
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
    stbtt_fontinfo info;
    uint8_t       *ttf_buffer;
    int            font_size;
    float          scale;
    int            ascent;
    int            descent;
    uint8_t       *glyph_cache[256];
    bool           used;
} font_t;

#endif
```

#### font.c

```c
#include "font.h"
#include "gamelib.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern font_t *g_fonts;

static font_t* font_get(int font_id)
{
    if (!g_fonts || font_id < 0 || font_id >= MAX_FONTS) return NULL;
    if (!g_fonts[font_id].used) return NULL;
    return &g_fonts[font_id];
}

int gamelib_font_load(gamelib_t *g, const uint8_t *ttf_data, int font_size)
{
    if (!g_fonts || !ttf_data || font_size <= 0) return -1;
    for (int i = 0; i < MAX_FONTS; i++) {
        if (!g_fonts[i].used) {
            font_t *f = &g_fonts[i];
            memset(f, 0, sizeof(*f));
            f->ttf_buffer = (uint8_t*)ttf_data;
            if (!stbtt_InitFont(&f->info, f->ttf_buffer, stbtt_GetFontOffsetForIndex(f->ttf_buffer, 0)))
                return -1;
            f->font_size = font_size;
            f->scale = stbtt_ScaleForPixelHeight(&f->info, (float)font_size);
            int ascent, descent, linegap;
            stbtt_GetFontVMetrics(&f->info, &ascent, &descent, &linegap);
            f->ascent = (int)(ascent * f->scale);
            f->descent = (int)(descent * f->scale);
            f->used = true;
            return i;
        }
    }
    return -1;
}

void gamelib_font_free(gamelib_t *g, int font_id)
{
    font_t *f = font_get(font_id);
    if (!f) return;
    for (int i = 0; i < 256; i++) { free(f->glyph_cache[i]); f->glyph_cache[i] = NULL; }
    memset(f, 0, sizeof(*f));
}

void gamelib_draw_text_font(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c, int font_id)
{
    font_t *f = font_get(font_id);
    if (!f || !s) return;
    int pen_x = x;
    while (*s) {
        int ch = (unsigned char)*s;
        if (!f->glyph_cache[ch]) {
            int w, h, xoff, yoff;
            unsigned char *bmp = stbtt_GetCodepointBitmap(&f->info, 0, f->scale, ch, &w, &h, &xoff, &yoff);
            f->glyph_cache[ch] = bmp;
        }
        uint8_t *bmp = f->glyph_cache[ch];
        if (bmp) {
            int w, h, xoff, yoff;
            stbtt_GetCodepointBox(&f->info, ch, &xoff, &yoff, NULL, NULL);
            stbtt_GetCodepointHMetrics(&f->info, ch, &w, NULL);
            /* re-get bitmap size */
            int bw, bh;
            {
                int x0, y0, x1, y1;
                stbtt_GetCodepointBitmapBox(&f->info, ch, f->scale, f->scale, &x0, &y0, &x1, &y1);
                bw = x1 - x0; bh = y1 - y0;
                for (int py = 0; py < bh; py++) {
                    for (int px = 0; px < bw; px++) {
                        uint8_t alpha = bmp[py * bw + px];
                        if (alpha > 0) {
                            int dx = pen_x + x0 + px;
                            int dy = y + f->ascent + y0 + py;
                            gamelib_set_pixel(g, dx, dy, c);
                        }
                    }
                }
            }
            pen_x += (int)(w * f->scale);
        }
        s++;
    }
}

int gamelib_get_text_width_font(gamelib_t *g, const char *s, int font_id)
{
    font_t *f = font_get(font_id);
    if (!f || !s) return 0;
    int width = 0;
    while (*s) {
        int ch = (unsigned char)*s;
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&f->info, ch, &adv, &lsb);
        width += (int)(adv * f->scale);
        s++;
    }
    return width;
}

void gamelib_draw_printf_font(gamelib_t *g, int x, int y, gamelib_color_t c, int font_id, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gamelib_draw_text_font(g, x, y, buf, c, font_id);
}
```

---

### Task 3: 修改 gamelib.h + gamelib.c — 集成

**Files:**
- Modify: `components/gamelib_core/gamelib.h`
- Modify: `components/gamelib_core/gamelib.c`

#### gamelib.h 修改

在 `#define MAX_SPRITES 64` 之后添加：

```c
#define MAX_TILEMAPS 8
#define SPRITE_COLORKEY 0x04
```

在 `typedef struct { ... } sprite_t;` 之后添加 `#include "tilemap.h"` 和 `#include "font.h"`。

在 gamelib_t struct 中添加（在 `sprite_t sprites[MAX_SPRITES];` 之后）：

```c
    tilemap_t    tilemaps[MAX_TILEMAPS];
    font_t       fonts[MAX_FONTS];
```

在 `gamelib.h` 末尾 `#endif` 之前声明全局变量：

```c
extern tilemap_t *g_tilemaps;
extern font_t    *g_fonts;
```

添加所有新 API 声明（Tilemap 12 个 + Font 5 个 + Grid 2 个）。

#### gamelib.c 修改

在 `gamelib_hal_t g_hal;` 之后添加：

```c
tilemap_t *g_tilemaps = NULL;
font_t    *g_fonts = NULL;
```

在 `gamelib_init()` 中分配全局数组：

```c
g_tilemaps = (tilemap_t*)calloc(MAX_TILEMAPS, sizeof(tilemap_t));
g_fonts = (font_t*)calloc(MAX_FONTS, sizeof(font_t));
```

在 `gamelib_deinit()` 中释放。

添加 `gamelib_draw_grid` 和 `gamelib_fill_cell` 实现：

```c
void gamelib_draw_grid(gamelib_t *g, int ox, int oy, int rows, int cols, int cell_size, gamelib_color_t c)
{
    for (int i = 0; i <= cols; i++)
        gamelib_draw_line(g, ox + i * cell_size, oy, ox + i * cell_size, oy + rows * cell_size, c);
    for (int i = 0; i <= rows; i++)
        gamelib_draw_line(g, ox, oy + i * cell_size, ox + cols * cell_size, oy + i * cell_size, c);
}

void gamelib_fill_cell(gamelib_t *g, int ox, int oy, int row, int col, int cell_size, gamelib_color_t c)
{
    gamelib_fill_rect(g, ox + col * cell_size, oy + row * cell_size, cell_size, cell_size, c);
}
```

---

### Task 4: 修改 sprite.c — 添加 SPRITE_COLORKEY 支持

**Files:**
- Modify: `components/gamelib_core/sprite.c`

在 `gamelib_draw_sprite_ex` 和 `gamelib_draw_sprite_frame_scaled` 中添加 color_key 检查逻辑。将现有的 `if (sp->has_color_key && c == sp->color_key) continue;` 逻辑也加到 flags 版本中。当 `flags & SPRITE_COLORKEY` 时才启用 color_key 检查（否则即使精灵有 color_key 也不跳过）。

实际修改：在两个函数的 color_key 检查前加上 `(flags & SPRITE_COLORKEY)` 条件。

现有代码：
```c
if (sp->has_color_key && c == sp->color_key) continue;
```

改为：
```c
if ((flags & SPRITE_COLORKEY) && sp->has_color_key && c == sp->color_key) continue;
```

---

### Task 5: 更新 main.c — 演示

**Files:**
- Modify: `main/main.c`

保留所有现有 MVE v4 功能，在右侧新增 mini 瓦片地图演示区（5×5 tilemap 用 procedural tileset）。

由于没有实际 TTF 文件，字体演示暂用注释标记，等用户准备好字体文件后开启。

---

### 变更总结

| 文件 | 操作 | 行数 |
|------|------|------|
| `tilemap.h` | 新建 | +25 |
| `tilemap.c` | 新建 | +140 |
| `font.h` | 新建 | +25 |
| `font.c` | 新建 | +120 |
| `stb_truetype.h` | 新建 | ~3000 (外部) |
| `gamelib.h` | 修改 | +30 |
| `gamelib.c` | 修改 | +30 |
| `sprite.c` | 修改 | +2 (SPRITE_COLORKEY) |
| `main.c` | 修改 | ~50 |
