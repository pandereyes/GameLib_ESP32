# PNG 加载与 Alpha 透明支持 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 GameLib 增加 PNG 加载和 Alpha 半透明混合支持，解决 BMP 贴图无透明通道的问题。

**Architecture:** 从 LVGL 复制 lodepng 库并剥离 LVGL 依赖，新增 `gamelib_sprite_load_png()` API，扩展 `sprite_t` 结构增加 alpha 数组，5 个绘制函数追加 Alpha 混合管线。

**Tech Stack:** C, lodepng (20201017), RGB565, ESP32-S3

---

### Task 1: 复制 lodepng 并剥离 LVGL 依赖

**Files:**
- Copy: `managed_components/lvgl__lvgl/src/extra/libs/png/lodepng.c` → `components/gamelib_core/lodepng.c`
- Copy: `managed_components/lvgl__lvgl/src/extra/libs/png/lodepng.h` → `components/gamelib_core/lodepng.h`

- [ ] **Step 1: 复制 lodepng.c 并去除 LVGL 依赖**

复制文件后，对 `lodepng.c` 做以下替换：

(1) 将文件首部的 `#if LV_USE_PNG` 替换为 `#if 1`
```
Grep location: 第 32 行 `#if LV_USE_PNG` → `#if 1`
```

(2) 将 LVGL 内存函数替换为标准 C：
- 全文替换 `lv_mem_alloc(size)` → `malloc(size)`
- 全文替换 `lv_mem_realloc(ptr, new_size)` → `realloc(ptr, new_size)`
- 全文替换 `lv_mem_free(ptr)` → `free(ptr)`
- 全文替换 `lv_memcpy(dst, src, size)` → `memcpy(dst, src, size)`
- 全文替换 `lv_memset(dst, value, num)` → `memset(dst, value, num)`

(3) 将 `LV_UNUSED(expected_size);` (第 2311 行附近) 替换为 `(void)(expected_size);`

(4) 文件 I/O 函数 `lodepng_load_file` / `lodepng_save_file`: 将 `lv_fs_*` 替换为标准 C：
第 344-354 行附近，替换为：
```c
static unsigned lodepng_load_file_internal(unsigned char** out, size_t* outsize, const char* filename) {
  FILE* f = fopen(filename, "rb");
  if(!f) return 78;
  fseek(f, 0, SEEK_END);
  long size_ = ftell(f);
  if(size_ < 0) { fclose(f); return 78; }
  size_t size = (size_t)size_;
  fseek(f, 0, SEEK_SET);
  *out = (unsigned char*)malloc(size);
  if(!*out) { fclose(f); return 83; }
  size_t br = fread(*out, 1, size, f);
  fclose(f);
  if(br != size) { free(*out); return 78; }
  *outsize = size;
  return 0;
}
```

第 385-391 行附近，替换为：
```c
static unsigned lodepng_save_file_internal(const unsigned char* buffer, size_t buffersize, const char* filename) {
  FILE* f = fopen(filename, "wb");
  if(!f) return 79;
  size_t bw = fwrite(buffer, 1, buffersize, f);
  fclose(f);
  if(bw != buffersize) return 79;
  return 0;
}
```

(5) 文件末尾 `#endif /*LV_USE_PNG*/` 替换为 `#endif /* LV_USE_PNG */`

- [ ] **Step 2: 复制 lodepng.h 并去除 LVGL 依赖**

对 `lodepng.h` 做以下替换：

(1) 第 29 行 `#include <string.h> /*for size_t*/` 后追加 `#include <stdlib.h>`

(2) 第 31 行 `#include "../../../lvgl.h"` 替换为 `#include <stdio.h>`

(3) 第 32 行 `#if LV_USE_PNG` 替换为 `#if 1`

(4) 第 1088 行 `#endif /*LV_USE_PNG*/` 替换为 `#endif`

- [ ] **Step 3: 编译验证 lodepng 文件无错误**

```bash
cd "e:\1.work\4.HUAYING\1.project\11.GameLib_MCU\1.source\gamelib_for_esp32"
idf.py build 2>&1 | tail -20
```

期望: 编译通过（lodepng.o 被编译进 libgamelib_core.a，虽然尚未被引用）

- [ ] **Step 4: 提交**

```bash
git add components/gamelib_core/lodepng.c components/gamelib_core/lodepng.h components/gamelib_core/CMakeLists.txt
git commit -m "feat: 添加 lodepng PNG 解码库，剥离 LVGL 依赖"
```

---

### Task 2: 修改 gamelib.h 数据结构与 API

**Files:**
- Modify: `components/gamelib_core/gamelib.h`

- [ ] **Step 1: 新增 SPRITE_ALPHA 标志**

在第 23 行 `#define SPRITE_COLORKEY 0x04` 后追加：

```c
#define SPRITE_ALPHA    0x08
```

- [ ] **Step 2: sprite_t 增加 alpha 字段**

在 `sprite_t` 结构体末尾（`bool has_color_key;` 之后，`} sprite_t;` 之前）插入：

```c
    uint8_t *alpha;              /* Alpha 数组，NULL = 无 alpha */
```

完整结构体变为：

```c
typedef struct {
    gamelib_color_t *pixels;
    int  width;
    int  height;
    bool used;
    gamelib_color_t color_key;
    bool has_color_key;
    uint8_t *alpha;
} sprite_t;
```

- [ ] **Step 3: 新增 PNG 加载 API 声明**

在第 127 行 `int  gamelib_sprite_load_bmp(...)` 之后追加：

```c
int  gamelib_sprite_load_png(gamelib_t *g, const uint8_t *data, size_t len);
```

- [ ] **Step 4: 提交**

```bash
git add components/gamelib_core/gamelib.h
git commit -m "feat: 添加 sprite_t.alpha 字段、SPRITE_ALPHA 标志和 gamelib_sprite_load_png API"
```

---

### Task 3: 创建 png.c PNG 加载实现

**Files:**
- Create: `components/gamelib_core/png.c`

- [ ] **Step 1: 编写 png.c**

```c
#include "gamelib.h"
#include "lodepng.h"
#include <stdlib.h>
#include <string.h>

int gamelib_sprite_load_png(gamelib_t *g, const uint8_t *data, size_t len)
{
    unsigned char *decoded = NULL;
    unsigned width = 0, height = 0;

    if (!data || len == 0) return -1;

    unsigned error = lodepng_decode32(&decoded, &width, &height, data, len);
    if (error) {
        if (decoded) free(decoded);
        return -1;
    }

    int sprite_id = gamelib_sprite_create(g, (int)width, (int)height);
    if (sprite_id < 0) {
        free(decoded);
        return -1;
    }

    /* 分配 alpha 数组 */
    size_t pixel_count = (size_t)width * height;
    g->sprites[sprite_id].alpha = (uint8_t *)malloc(pixel_count);
    if (!g->sprites[sprite_id].alpha) {
        free(decoded);
        gamelib_sprite_free(g, sprite_id);
        return -1;
    }

    /* 逐像素转换 ARGB8888 → RGB565 + Alpha */
    for (unsigned i = 0; i < pixel_count; i++) {
        uint8_t r = decoded[i * 4 + 0];
        uint8_t g = decoded[i * 4 + 1];
        uint8_t b = decoded[i * 4 + 2];
        uint8_t a = decoded[i * 4 + 3];

        g->sprites[sprite_id].alpha[i] = a;

        if (a == 0) {
            g->sprites[sprite_id].pixels[i] = COLOR_NONE;
        } else {
            gamelib_color_t c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            c = (c >> 8) | (c << 8);
            g->sprites[sprite_id].pixels[i] = c;
        }
    }

    free(decoded);

    /* 自动设置 color key 兼容 SPRITE_COLORKEY 模式 */
    gamelib_sprite_set_color_key(g, sprite_id, COLOR_NONE);

    return sprite_id;
}
```

- [ ] **Step 2: 提交**

```bash
git add components/gamelib_core/png.c
git commit -m "feat: 实现 gamelib_sprite_load_png PNG 加载函数"
```

---

### Task 4: 为绘制函数追加 Alpha 混合

**Files:**
- Modify: `components/gamelib_core/sprite.c`

**Alpha 混合宏** — 在所有绘制函数之前（`sprite.c` 顶部 `#include` 之后）插入：

```c
/* Alpha 混合: src * a + dst * (256-a) >> 8, 3 通道独立，无除法 */
#define BLEND_ALPHA(src, dst, a) do { \
    if ((a) == 0) { (dst) = (src); } \
    else { \
        unsigned sr = ((src) >> 11) & 0x1F, dr = ((dst) >> 11) & 0x1F; \
        unsigned sg = ((src) >> 5)  & 0x3F, dg = ((dst) >> 5)  & 0x3F; \
        unsigned sb = (src) & 0x1F,        db = (dst) & 0x1F; \
        unsigned ia = (a), ib = 256 - (a); \
        unsigned or_ = (sr * ia + dr * ib) >> 8; \
        unsigned og = (sg * ia + dg * ib) >> 8; \
        unsigned ob = (sb * ia + db * ib) >> 8; \
        (dst) = (gamelib_color_t)((or_ << 11) | (og << 5) | ob); \
    } \
} while(0)
```

- [ ] **Step 1: gamelib_draw_sprite_region 和 gamelib_draw_sprite 保持不变**

这两个函数不接收 `flags` 参数，保持现有 color key 逻辑不变。Alpha 混合通过下面 5 个函数实现。

- [ ] **Step 2: 修改 gamelib_draw_sprite_ex (第 102-123 行)**

将第 114-120 行替换为：

```c
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                gamelib_color_t *dst = &fb->pixels[py * fb->width + px];
                if ((flags & SPRITE_ALPHA) && sp->alpha) {
                    uint8_t a = sp->alpha[src_y * sw + src_x];
                    BLEND_ALPHA(c, *dst, a);
                } else {
                    *dst = c;
                }
            }
```

- [ ] **Step 3: 修改 gamelib_draw_sprite_scaled (第 126-149 行)**

将第 141-145 行替换为：

```c
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                gamelib_color_t *dst = &fb->pixels[py * fb->width + px];
                if (sp->alpha) {
                    BLEND_ALPHA(c, *dst, sp->alpha[src_y * sw + src_x]);
                } else {
                    *dst = c;
                }
            }
```

注意: `draw_sprite_scaled` 不接收 `flags` 参数。若精灵有 alpha 数据则自动混合。

- [ ] **Step 4: 修改 gamelib_draw_sprite_frame_scaled (第 151-181 行)**

将第 171-178 行替换为：

```c
            int px = dst_x + col;
            int py = dst_y + row;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                gamelib_color_t *dst = &fb->pixels[py * fb->width + px];
                if ((flags & SPRITE_ALPHA) && sp->alpha) {
                    uint8_t a = sp->alpha[src_y * sheet_w + src_x];
                    BLEND_ALPHA(c, *dst, a);
                } else {
                    *dst = c;
                }
            }
```

- [ ] **Step 5: 修改 gamelib_draw_sprite_rotated (第 183-225 行)**

将第 218-221 行替换为：

```c
            int px = cx + dx, py = cy + dy;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                gamelib_color_t *dst = &fb->pixels[py * fb->width + px];
                if ((flags & SPRITE_ALPHA) && sp->alpha) {
                    uint8_t a = sp->alpha[sy * sw + sx];
                    BLEND_ALPHA(c, *dst, a);
                } else {
                    *dst = c;
                }
            }
```

- [ ] **Step 6: 修改 gamelib_draw_sprite_frame_rotated (第 227-271 行)**

将第 263-269 行替换为：

```c
            int px = cx + dx, py = cy + dy;
            if (px >= fb->clip_x && px < fb->clip_x + fb->clip_w &&
                py >= fb->clip_y && py < fb->clip_y + fb->clip_h) {
                gamelib_color_t *dst = &fb->pixels[py * fb->width + px];
                if ((flags & SPRITE_ALPHA) && sp->alpha) {
                    uint8_t a = sp->alpha[sy * sheet_w + (src_ox + sx)];
                    BLEND_ALPHA(c, *dst, a);
                } else {
                    *dst = c;
                }
            }
```

- [ ] **Step 7: 修改 gamelib_sprite_free (第 34-42 行)**

在 `free(g->sprites[id].pixels);` 之后追加 alpha 释放：

```c
            if (g->sprites[id].alpha) {
                free(g->sprites[id].alpha);
                g->sprites[id].alpha = NULL;
            }
```

- [ ] **Step 8: 提交**

```bash
git add components/gamelib_core/sprite.c
git commit -m "feat: 所有绘制函数追加 Alpha 混合管线"
```

---

### Task 5: 更新 CMakeLists.txt

**Files:**
- Modify: `components/gamelib_core/CMakeLists.txt`

- [ ] **Step 1: 添加新源文件**

将第 1-4 行：
```cmake
idf_component_register(
    SRCS "ui.c" "font.c" "tilemap.c" "wav.c" "gamelib.c" "framebuffer.c" "draw.c" "text.c" "sprite.c" "bmp.c" "audio.c"
```

修改为：
```cmake
idf_component_register(
    SRCS "ui.c" "font.c" "tilemap.c" "wav.c" "gamelib.c" "framebuffer.c" "draw.c" "text.c" "sprite.c" "bmp.c" "png.c" "lodepng.c" "audio.c"
```

- [ ] **Step 2: 提交**

```bash
git add components/gamelib_core/CMakeLists.txt
git commit -m "build: CMakeLists 添加 png.c 和 lodepng.c"
```

---

### Task 6: 构建验证

- [ ] **Step 1: 完整构建**

```bash
cd "e:\1.work\4.HUAYING\1.project\11.GameLib_MCU\1.source\gamelib_for_esp32"
idf.py build 2>&1 | tail -30
```

期望: 编译通过，无 warning，无 error。

- [ ] **Step 2: 验证 flash 占用**

```bash
idf.py size-components 2>&1 | grep -E "gamelib_core|lodepng|Total"
```

期望: lodepng 占用约 30-50KB flash（zlib + 解码器）。

---

### Task 7: 更新 demo_14 使用 PNG

**Files:**
- Modify: `main/demos/demo_14_space_shooter.c`
- (可选) 提供 PNG 测试资源

- [ ] **Step 1: 修改 load_sprite_fallback 使用 PNG 优先加载**

将第 47-54 行的 `load_sprite_fallback` 函数修改为优先尝试 PNG：

```c
static int load_sprite_fallback(gamelib_t *g, const char *bmp_path, const char *png_path) {
    size_t len;
    uint8_t *buf;
    /* 优先尝试 PNG */
    if (png_path) {
        buf = load_file_data(g, png_path, &len);
        if (buf) {
            int id = gamelib_sprite_load_png(g, buf, len);
            free(buf);
            if (id >= 0) return id;
        }
    }
    /* 回退到 BMP */
    buf = load_file_data(g, bmp_path, &len);
    if (!buf) return -1;
    int id = gamelib_sprite_load_bmp(g, buf, len);
    free(buf);
    return id;
}
```

- [ ] **Step 2: 更新精灵加载调用**

将第 139 行 `load_sprite_fallback(g, "assert/plane0.bmp")` 改为：
```c
int sprPlayer = load_sprite_fallback(g, "assert/plane0.bmp", "assert/plane0.png");
```

将第 144 行 `load_sprite_fallback(g, "assert/bullet.bmp")` 改为：
```c
int sprBullet = load_sprite_fallback(g, "assert/bullet.bmp", "assert/bullet.png");
```

将第 145 行 `load_sprite_fallback(g, "assert/explosion.bmp")` 改为：
```c
int sprExplosion = load_sprite_fallback(g, "assert/explosion.bmp", "assert/explosion.png");
```

- [ ] **Step 3: 绘制调用启用 SPRITE_ALPHA**

修改精灵绘制调用，追加 `SPRITE_ALPHA` 标志：
- 第 369-370 行 `gamelib_draw_sprite_scaled(..., sprBullet, ...)` → 参数不变（scaled 自动 alpha）
- 第 387 行 `gamelib_draw_sprite_ex(g, spr, ..., SPRITE_COLORKEY)` → 改为 `SPRITE_COLORKEY | SPRITE_ALPHA`
- 第 396-397 行 `gamelib_draw_sprite_frame_scaled(..., SPRITE_COLORKEY)` → 改为 `SPRITE_COLORKEY | SPRITE_ALPHA`
- 第 410 行 `gamelib_draw_sprite_ex(g, sprPlayer, ..., SPRITE_COLORKEY)` → 改为 `SPRITE_COLORKEY | SPRITE_ALPHA`

**注意:** 手动创建的精灵（`create_player_sprite` / `create_enemy_sprite`）没有 alpha 数组，`SPRITE_ALPHA && sp->alpha` 条件中 `sp->alpha` 为 NULL 因此不会走混合路径。安全。

- [ ] **Step 4: 提交**

```bash
git add main/demos/demo_14_space_shooter.c
git commit -m "feat: demo_14 使用 PNG 加载和 Alpha 混合"
```
