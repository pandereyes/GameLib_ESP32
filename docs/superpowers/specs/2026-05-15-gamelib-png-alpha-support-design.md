# GameLib ESP32-S3：PNG 加载与 Alpha 透明支持

**日期**: 2026-05-15
**状态**: 设计完成，待实施
**前置**: BMP 加载已支持，color key 机制已存在

---

## 1. 问题背景

当前 BMP 加载 (`gamelib_sprite_load_bmp`) 不支持透明，即使 32 位 BMP 的 Alpha 通道也被丢弃。导致贴图显示为带背景色的色块。PNG 格式支持真正的 Alpha 透明，配合 Alpha 混合可在游戏中实现平滑边缘和特效。

## 2. 方案概要

从 LVGL managed_components 复制 lodepng 库（纯 C 单文件），新增 `gamelib_sprite_load_png()` API，扩展精灵结构支持 Alpha 通道，所有绘制函数追加 Alpha 混合。

## 3. API

```c
/* 新 API：从内存中的 PNG 数据创建精灵，自动处理 Alpha */
int gamelib_sprite_load_png(gamelib_t *g, const uint8_t *data, size_t len);

/* 新 flag */
#define SPRITE_ALPHA  0x08
```

调用方式与 BMP 完全对称，替换函数名即可。

## 4. 数据结构变更

```c
typedef struct {
    gamelib_color_t *pixels;
    int width, height;
    bool used;
    bool has_color_key;
    gamelib_color_t color_key;
    uint8_t *alpha;          // 新增：Alpha 数组，NULL 表示无 alpha
} sprite_t;
```

## 5. PNG 解码流程 (`png.c`)

```
PNG 数据 → lodepng_decode32() → ARGB8888 逐像素输出
  ├─ Alpha == 0   → pixels[i] = COLOR_NONE, alpha[i] = 0
  ├─ Alpha == 255 → pixels[i] = RGB565,         alpha[i] = 255
  └─ 其他         → pixels[i] = RGB565,         alpha[i] = Alpha 原值
自动设置 color_key = COLOR_NONE，分配 alpha 数组
```

## 6. Alpha 混合绘制管线

所有绘制函数（6 个）追加 Alpha 混合逻辑：

```
对每个像素:
  if (SPRITE_ALPHA && sp->alpha):
      src_r5 = src >> 11; dst_r5 = dst >> 11;
      src_g6 = (src >> 5) & 0x3F; dst_g6 = (dst >> 5) & 0x3F;
      src_b5 = src & 0x1F; dst_b5 = dst & 0x1F;
      out_r = (src_r5 * a + dst_r5 * (256-a)) >> 8;
      out_g = (src_g6 * a + dst_g6 * (256-a)) >> 8;
      out_b = (src_b5 * a + dst_b5 * (256-a)) >> 8;
      写入 FB = (out_r << 11) | (out_g << 5) | out_b;
  else:
      走现有 color key 逻辑（不变）
```

混合公式用 256 分母，位移代替除法，无浮点运算。每像素约 12 次整数运算。

## 7. 需要修改的绘制函数

`sprite.c` 中 6 个函数，每个函数在写入 FB 前追加 Alpha 混合分支：

- `gamelib_draw_sprite_region`
- `gamelib_draw_sprite_ex`
- `gamelib_draw_sprite_scaled`
- `gamelib_draw_sprite_frame_scaled`
- `gamelib_draw_sprite_rotated`
- `gamelib_draw_sprite_frame_rotated`

## 8. 性能

64×64 精灵全半透明约 4096 像素，每帧 Alpha 混合开销约 50K 指令，ESP32-S3 @ 160MHz 下约 0.3ms。PNG 解码仅在资源加载时执行一次（32×32 精灵 < 5ms，64×64 约 15ms）。

## 9. 影响范围

| 文件 | 动作 |
|------|------|
| `components/gamelib_core/lodepng.c` | 复制自 LVGL managed_components |
| `components/gamelib_core/lodepng.h` | 复制自 LVGL managed_components |
| `components/gamelib_core/png.c` | 新建 ~80 行 |
| `components/gamelib_core/sprite.c` | 修改 ~60 行（6 个函数追加混合） |
| `components/gamelib_core/gamelib.h` | 修改：sprite_t 加 alpha 字段，新 API 声明，SPRITE_ALPHA flag |
| `components/gamelib_core/CMakeLists.txt` | 添加 lodepng.c png.c |

## 10. 向后兼容

- BMP 加载不变，sprite_t.alpha 保持 NULL
- SPRITE_COLORKEY 逻辑不变
- 无 alpha 数据的精灵行为完全不变
