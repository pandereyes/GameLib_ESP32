# GameLib ESP32-S3 第五批移植：焦点式 UI 控件

**日期**: 2026-05-11
**状态**: 设计完成，待实施
**前置**: Batch4 已验证通过
**范围**: button / checkbox / radio_box / toggle_button 四个即时模式 UI 控件，焦点导航

---

## 1. 交互模型

游戏机经典模式：D-Pad 切换焦点，A 确认，B 取消。

```
D-Pad UP/DOWN  → 焦点上下移动
KEY_A           → 激活当前焦点控件
KEY_B           → 取消/返回

焦点控件自动绘制高亮边框 (COLOR_GOLD)
```

## 2. API

```c
/* 每帧 UI 管理 */
void gamelib_ui_begin(gamelib_t *g);
void gamelib_ui_end(gamelib_t *g);

/* 即时模式控件 */
bool gamelib_button(gamelib_t *g, int x, int y, int w, int h,
                    const char *label, gamelib_color_t color);
bool gamelib_checkbox(gamelib_t *g, int x, int y, const char *label, bool *value);
bool gamelib_radio_box(gamelib_t *g, int x, int y, const char *label, int *value, int group_value);
bool gamelib_toggle_button(gamelib_t *g, int x, int y, int w, int h,
                           const char *label, bool *value, gamelib_color_t color);
```

## 3. 焦点管理

每帧重建焦点链：

```
ui_begin() → 清空焦点数组
控件调用 → 顺序注册到焦点数组
ui_end()  → 处理 UP/DOWN 移动焦点，A 键激活当前，B 键取消
```

内部控制使用 gamelib_is_key_pressed 检测按键。

## 4. 控件行为

| 控件 | 绘制 | 触发时 |
|------|------|--------|
| Button | 填充矩形 + 居中文字 | 返回 true |
| Checkbox | [x]/[ ] + label | 翻转 *value |
| RadioBox | (o)/( ) + label | 设 *value = group_value |
| ToggleButton | 凹陷/凸起矩形 + label | 翻转 *value |

## 5. 数据结构

```c
#define MAX_UI_ELEMENTS 32

typedef enum { UI_BUTTON, UI_CHECKBOX, UI_RADIO, UI_TOGGLE } ui_type_t;

typedef struct {
    int x, y, w, h;
    ui_type_t type;
    int group_value;
    union {
        gamelib_color_t *color;
        bool *bool_val;
        int  *int_val;
    };
} ui_elem_t;
```

## 6. 文件清单

| 文件 | 操作 | 行数 |
|------|------|------|
| `ui.h` | 新建 | +35 |
| `ui.c` | 新建 | ~250 |
| `gamelib.h` | 修改 | +6 声明 |
| `gamelib.c` | 修改 | ui_begin/end 调用 |
| `main.c` | 重写 | 纯 UI 演示 |

## 7. main.c

只用 UI 控件演示，去掉所有 sprite/color/tilemap 演示代码。
