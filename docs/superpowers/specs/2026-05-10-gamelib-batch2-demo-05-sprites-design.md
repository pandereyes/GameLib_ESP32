# GameLib ESP32-S3 第二批移植：精灵翻转/缩放/帧动画

**日期**: 2026-05-10
**状态**: 设计完成，待实施
**前置**: Batch1 (demo 01-03) 已验证通过
**范围**: 新增 3 个精灵绘制 API 以支持 demo 05

---

## 1. 目标

新增精灵翻转、缩放、帧动画 API，支持 demo 05 全部功能。

---

## 2. 新增 API

### 2.1 翻转标志

```c
#define SPRITE_FLIP_H  1   /* 水平翻转 */
#define SPRITE_FLIP_V  2   /* 垂直翻转 */
```

可组合: `SPRITE_FLIP_H | SPRITE_FLIP_V` 同时翻转。

### 2.2 gamelib_draw_sprite_ex — 翻转绘制

```c
void gamelib_draw_sprite_ex(gamelib_t *g, int id, int x, int y, int flags);
```

翻转时像素索引映射:
- 正常: `src[row * sw + col]`
- H翻: `src[row * sw + (sw - 1 - col)]`
- V翻: `src[(sh - 1 - row) * sw + col]`
- H+V翻: `src[(sh - 1 - row) * sw + (sw - 1 - col)]`

### 2.3 gamelib_draw_sprite_scaled — 缩放绘制

```c
void gamelib_draw_sprite_scaled(gamelib_t *g, int id, int x, int y, int dst_w, int dst_h);
```

最近邻插值: `src_x = dst_col * sw / dst_w`, `src_y = dst_row * sh / dst_h`.

### 2.4 gamelib_draw_sprite_frame_scaled — 帧动画

```c
void gamelib_draw_sprite_frame_scaled(gamelib_t *g, int id, int x, int y,
                                       int fw, int fh, int frame, int dst_w, int dst_h,
                                       int flags);
```

帧布局: 水平排列，帧 N 源区域 = `(N * fw, 0, fw, fh)`.
参数 `flags` 支持 `SPRITE_FLIP_H` / `SPRITE_FLIP_V` 组合，传 0 表示不翻转。

---

## 3. 实现约束

- 缩放使用整数除法，避免浮点
- 所有函数尊重 sprite 的 color_key（跳过透明像素）
- 所有函数尊重 framebuffer 的 clip 区域
- 不做边界检查重排（由调用者保证坐标合法性，与现有 draw_sprite 行为一致）

---

## 4. 文件变更

| 文件 | 内容 |
|------|------|
| `gamelib.h` | +3 API 声明 + `SPRITE_FLIP_H/V` 宏 |
| `sprite.c` | +3 函数实现 (~60行) |
| `main.c` | 演示精灵翻转/缩放 |

---

## 5. 不做

- 精灵旋转 (batch 后续)
- 双线性插值
- 批量绘制
