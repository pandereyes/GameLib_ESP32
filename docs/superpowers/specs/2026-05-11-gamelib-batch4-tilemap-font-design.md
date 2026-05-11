# GameLib ESP32-S3 第四批：Tilemap + TTF 字体 + 网格绘图

**日期**: 2026-05-11
**状态**: 设计完成，待实施
**前置**: Batch1/2/3 已验证通过
**范围**: Tilemap 子系统、TTF 字体渲染（stb_truetype）、网格辅助函数

---

## 1. 目标

新增 Tilemap 子系统（12 API）、TTF 字体系统（4 API + stb_truetype）、网格绘图（2 API），支持 demo 09/10/11。

---

## 2. Tilemap 子系统

### 2.1 API

```c
#define MAX_TILEMAPS 8
#define SPRITE_COLORKEY 0x04

int  gamelib_tilemap_create(gamelib_t *g, int cols, int rows, int tile_size, int tileset_id);
void gamelib_tilemap_clear(gamelib_t *g, int map_id, int tile_id);
void gamelib_tilemap_set_tile(gamelib_t *g, int map_id, int col, int row, int tile_id);
void gamelib_tilemap_fill_rect(gamelib_t *g, int map_id, int col, int row, int w, int h, int tile_id);
int  gamelib_tilemap_get_cols(gamelib_t *g, int map_id);
int  gamelib_tilemap_get_rows(gamelib_t *g, int map_id);
int  gamelib_tilemap_get_tile_size(gamelib_t *g, int map_id);
void gamelib_tilemap_draw(gamelib_t *g, int map_id, int offset_x, int offset_y);
int  gamelib_tilemap_world_to_col(gamelib_t *g, int map_id, int world_x);
int  gamelib_tilemap_world_to_row(gamelib_t *g, int map_id, int world_y);
int  gamelib_tilemap_get_at_pixel(gamelib_t *g, int map_id, int world_x, int world_y);
void gamelib_tilemap_free(gamelib_t *g, int map_id);
```

### 2.2 数据结构

```c
typedef struct {
    int16_t *tiles;
    int      cols, rows, tile_size;
    int      tileset_id;
    bool     used;
} tilemap_t;
```

tiles 数组存储在 PSRAM（malloc），支持 -1 表示空瓦片。绘制时跳过 -1 瓦片。

### 2.3 渲染

遍历可见区域瓦片，通过 `draw_sprite_region` 从 tileset 精灵裁剪对应瓦片绘制。`offset_x/y` 实现摄像机滚动。

### 2.4 SPRITE_COLORKEY

新增 sprite 绘制标志 `SPRITE_COLORKEY = 0x04`，与 `SPRITE_FLIP_H/V` 可组合。设置后 draw_sprite_ex 和 draw_sprite_frame_scaled 自动应用 color_key。

---

## 3. TTF 字体系统

### 3.1 依赖

`stb_truetype.h` — 单头文件库，从 https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h 获取。放入 `components/gamelib_core/`。

### 3.2 API

```c
#define MAX_FONTS 4

int  gamelib_font_load(gamelib_t *g, const uint8_t *ttf_data, int font_size);
void gamelib_font_free(gamelib_t *g, int font_id);
void gamelib_draw_text_font(gamelib_t *g, int x, int y, const char *s, gamelib_color_t c, int font_id);
void gamelib_draw_printf_font(gamelib_t *g, int x, int y, gamelib_color_t c, int font_id, const char *fmt, ...);
int  gamelib_get_text_width_font(gamelib_t *g, const char *s, int font_id);
```

### 3.3 实现

- `font_load`: 解析 TTF 头，创建 stbtt_fontinfo，分配 glyph 缓存（256 slots, PSRAM）
- `draw_text_font`: 对每个字符调用 `stbtt_GetCodepointBitmap()` 光栅化到临时 buffer，写入 framebuffer
- 字形首次渲染后缓存位图，后续直接复用
- 字体文件通过 `xxd -i font.ttf` 嵌入 C 数组

---

## 4. 网格绘图

```c
void gamelib_draw_grid(gamelib_t *g, int ox, int oy, int rows, int cols, int cell_size, gamelib_color_t c);
void gamelib_fill_cell(gamelib_t *g, int ox, int oy, int row, int col, int cell_size, gamelib_color_t c);
```

使用已有 draw_line / fill_rect 实现。

---

## 5. gamelib_t 扩展

```c
tilemap_t tilemaps[MAX_TILEMAPS];  // 新增
```

---

## 6. 文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `tilemap.h` | 新建 | Tilemap API 声明和数据结构 |
| `tilemap.c` | 新建 | Tilemap 实现（~180行） |
| `stb_truetype.h` | 新建 | TTF 光栅化库（~3000行外部库） |
| `font.h` | 新建 | 字体 API 声明 |
| `font.c` | 新建 | 字体实现（~120行） |
| `gamelib.h` | 修改 | +20 API 声明 + SPRITE_COLORKEY + tilemap_t |
| `gamelib.c` | 修改 | gamelib_init 初始化 tilemaps，deinit 释放 |
| `sprite.c` | 修改 | draw_sprite_ex/draw_sprite_frame_scaled 支持 SPRITE_COLORKEY |
| `main.c` | 修改 | 演示 tilemap + 字体 |

---

## 7. 不做

- PNG 加载（tileset 使用 procedural 精灵或 BMP）
- show_message / show_mouse（需要窗口系统支持）
- 字体多尺寸混合（同一 font_id 固定尺寸）
