# GameLib ESP32-S3 第一批移植：支持 Demo 01-03

**日期**: 2026-05-10
**状态**: 设计完成，待实施
**前置**: MVE 已验证通过
**范围**: 新增 API 以支持 demo 01-03 全部功能

---

## 1. 目标

在不破坏 MVE 已有功能的前提下，补充以下 API 以支持前 3 个 demo:

- **时间类**: GetTime、GetDeltaTime、GetWidth、GetHeight
- **文字类**: DrawPrintf（格式化文字输出）
- **颜色类**: COLOR_RGB 宏 + 补充预定义颜色

Demo 04 暂不处理（依赖鼠标，ESP32 无此外设）。

---

## 2. 新增 API 设计

### 2.1 时间模块

```c
// 返回自 init 以来的运行秒数 (double)
double gamelib_get_time(gamelib_t *g);

// 返回上一帧的间隔秒数 (double)，首帧返回 0
double gamelib_get_delta_time(gamelib_t *g);

// 返回 framebuffer 宽度
int gamelib_get_width(gamelib_t *g);

// 返回 framebuffer 高度
int gamelib_get_height(gamelib_t *g);
```

**实现要点**:
- `gamelib_t` 需新增 `start_time` 字段（记录 init 时刻的毫秒值）
- `delta_time` 在 `gamelib_wait_frame()` 中计算
- `width/height` 在 `gamelib_init()` 中存储到 gamelib_t

### 2.2 格式化文字

```c
// printf 风格格式化文字输出
void gamelib_draw_printf(gamelib_t *g, int x, int y, gamelib_color_t c, const char *fmt, ...);
```

**实现要点**:
- 使用 `vsnprintf` 格式化到栈上缓冲区（256 字节足够嵌入式使用）
- 格式化后调用已有的 `gamelib_draw_text`
- 需要 `#include <stdio.h>` 和 `#include <stdarg.h>`

### 2.3 颜色宏

所有颜色值采用 **字节交换后的 RGB565** 格式，适配 SPI DMA 传输（MSB 先发，与 ST7789 一致）。

```
字节0 (SPI先发): RRRRRGGG  = (r & 0xF8) | (g >> 5)
字节1 (SPI后发): GGGBBBBB  = ((g & 0x1C) << 3) | (b >> 3)
```

```c
// RGB565 颜色合成宏（字节交换后，适配 SPI DMA）
#define COLOR_RGB(r, g, b) \
    ((uint16_t)(((r) & 0xF8) | ((g) >> 5)) | \
     ((uint16_t)(((g) & 0x1C) << 3) | ((b) >> 3)) << 8)
```

**不实现 COLOR_ARGB / Alpha 混合**: demo 03 中的 alpha 混合在嵌入式屏幕（无 GPU）上开销过大，且 demo 03 即使不做 alpha 混合也能正常展示形状功能。

### 2.4 补充预定义颜色

在 `hal_types.h` 新增（所有值已通过 SPI DMA 字节交换公式验证）:

```c
/* 已有 (保持不变) */
#define COLOR_NONE       0x0000
#define COLOR_BLACK      0x0000
#define COLOR_RED        0x00F8
#define COLOR_GREEN      0xE007
#define COLOR_BLUE       0x1F00
#define COLOR_YELLOW     0xE0FF
#define COLOR_CYAN       0xFF07
#define COLOR_MAGENTA    0x1FF8
#define COLOR_GRAY       0x1084
#define COLOR_DARKGRAY   0x0842
#define COLOR_LIGHTGRAY  0x18C6
#define COLOR_WHITE      0xFFFF

/* 新增 */
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

---

## 3. 数据结构变更

`gamelib_t` 新增字段（`gamelib.h`）:

```c
typedef struct {
    framebuffer_t fb;
    bool          running;
    double        delta_time;    // 已有
    double        fps;           // 已有
    double        frame_start;   // 已有
    double        start_time;    // ✨ 新增：init 时刻时间戳(秒)
    int           target_fps;    // 已有

    sprite_t      sprites[MAX_SPRITES];
    uint8_t       keystate[KEY_COUNT];
    uint8_t       key_prev[KEY_COUNT];
    int           mouse_x, mouse_y;
} gamelib_t;
```

---

## 4. 文件变更清单

| 文件 | 变更类型 | 内容 |
|------|---------|------|
| `hal_types.h` | 修改 | 补充颜色宏 + COLOR_RGB 宏 |
| `gamelib.h` | 修改 | 新增 API 声明 + gamelib_t 新增 start_time |
| `gamelib.c` | 修改 | 实现 GetTime/GetDeltaTime/GetWidth/GetHeight/DrawPrintf |

---

## 5. 向后兼容

- 所有现有 API 签名不变
- gamelib_t 新增字段为追加，不改变已有字段布局（C 兼容）
- 现有的 `main.c` (MVE) 无需任何修改即可编译运行

---

## 6. 不做

- **COLOR_ARGB / Alpha 混合**: 嵌入式平台无 GPU 加速，逐像素 alpha 混合不实用
- **鼠标相关 API**: ESP32 无鼠标，demo 04 整体跳过
- **DrawSpriteEx / DrawSpriteScaled / DrawSpriteFrameScaled**: 放入第二批
- **WAV / 文件系统**: 放入第三批
