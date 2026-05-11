# GameLib ESP32-S3 Batch5: Focus-Based UI Controls

> **For agentic workers:** Use subagent-driven-development. No auto-commit.

**Goal:** 焦点式 UI 控件（button/checkbox/radio_box/toggle_button），D-Pad 导航。

**Architecture:** ui.c 维护焦点链数组，每帧 ui_begin() 清空、控件调用注册、ui_end() 处理按键导航。所有控件用内置 8x8 字体绘制标签。

---

### Task 1: 新建 ui.h + ui.c

**Files:**
- Create: `components/gamelib_core/ui.h`
- Create: `components/gamelib_core/ui.c`

#### ui.h

```c
#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include "hal_types.h"

#define MAX_UI_ELEMENTS 32

typedef enum { UI_BUTTON, UI_CHECKBOX, UI_RADIO, UI_TOGGLE } ui_type_t;

#endif
```

#### ui.c

```c
#include "ui.h"
#include "gamelib.h"
#include <string.h>

static int ui_focus = -1;
static int ui_count = 0;

static struct {
    int        x, y, w, h;
    ui_type_t  type;
    union {
        gamelib_color_t *color;
        bool *bval;
        int  *ival;
    } data;
    int         ival;
} ui_elems[MAX_UI_ELEMENTS];

void gamelib_ui_begin(gamelib_t *g)
{
    (void)g;
    ui_count = 0;
}

static int ui_add(gamelib_t *g, int x, int y, int w, int h, ui_type_t type)
{
    if (ui_count >= MAX_UI_ELEMENTS) return -1;
    int idx = ui_count++;
    ui_elems[idx].x = x;
    ui_elems[idx].y = y;
    ui_elems[idx].w = w;
    ui_elems[idx].h = h;
    ui_elems[idx].type = type;
    return idx;
}

void gamelib_ui_end(gamelib_t *g)
{
    if (ui_count == 0) return;

    /* Init focus if unset */
    if (ui_focus < 0 || ui_focus >= ui_count) ui_focus = 0;

    /* Navigate */
    if (gamelib_is_key_pressed(g, KEY_DOWN)) {
        ui_focus = (ui_focus + 1) % ui_count;
    }
    if (gamelib_is_key_pressed(g, KEY_UP)) {
        ui_focus = (ui_focus - 1 + ui_count) % ui_count;
    }
    if (gamelib_is_key_pressed(g, KEY_RIGHT)) {
        ui_focus = (ui_focus + 1) % ui_count;
    }
    if (gamelib_is_key_pressed(g, KEY_LEFT)) {
        ui_focus = (ui_focus - 1 + ui_count) % ui_count;
    }

    /* Draw focus highlight */
    if (ui_focus >= 0 && ui_focus < ui_count) {
        int fx = ui_elems[ui_focus].x - 2;
        int fy = ui_elems[ui_focus].y - 2;
        int fw = ui_elems[ui_focus].w + 4;
        int fh = ui_elems[ui_focus].h + 4;
        gamelib_draw_rect(g, fx, fy, fw, fh, COLOR_GOLD);
    }
}

static bool ui_activated(gamelib_t *g, int idx)
{
    if (idx == ui_focus && gamelib_is_key_pressed(g, KEY_A))
        return true;
    return false;
}

bool gamelib_button(gamelib_t *g, int x, int y, int w, int h, const char *label, gamelib_color_t color)
{
    int idx = ui_add(g, x, y, w, h, UI_BUTTON);
    if (idx < 0) return false;

    /* draw */
    gamelib_color_t bg = color;
    gamelib_fill_rect(g, x, y, w, h, bg);
    gamelib_draw_rect(g, x, y, w, h, COLOR_WHITE);

    int lbl_w = (int)strlen(label) * 8;
    int tx = x + (w - lbl_w) / 2;
    int ty = y + (h - 8) / 2;
    gamelib_draw_text(g, tx, ty, label, COLOR_WHITE);

    return ui_activated(g, idx);
}

bool gamelib_checkbox(gamelib_t *g, int x, int y, const char *label, bool *value)
{
    int idx = ui_add(g, x, y, 16, 14, UI_CHECKBOX);
    if (idx < 0) return false;
    ui_elems[idx].data.bval = value;

    gamelib_draw_rect(g, x, y, 14, 14, COLOR_WHITE);
    if (*value) {
        gamelib_draw_line(g, x + 2, y + 2, x + 11, y + 11, COLOR_WHITE);
        gamelib_draw_line(g, x + 11, y + 2, x + 2, y + 11, COLOR_WHITE);
    }
    gamelib_draw_text(g, x + 20, y + 3, label, COLOR_WHITE);

    if (ui_activated(g, idx)) {
        *value = !*value;
        return true;
    }
    return false;
}

bool gamelib_radio_box(gamelib_t *g, int x, int y, const char *label, int *value, int group_value)
{
    int idx = ui_add(g, x, y, 14, 14, UI_RADIO);
    if (idx < 0) return false;
    ui_elems[idx].data.ival = value;
    ui_elems[idx].ival = group_value;

    gamelib_draw_circle(g, x + 7, y + 7, 7, COLOR_WHITE);
    if (*value == group_value)
        gamelib_fill_circle(g, x + 7, y + 7, 3, COLOR_WHITE);
    gamelib_draw_text(g, x + 20, y + 3, label, COLOR_WHITE);

    if (ui_activated(g, idx)) {
        *value = group_value;
        return true;
    }
    return false;
}

bool gamelib_toggle_button(gamelib_t *g, int x, int y, int w, int h, const char *label, bool *value, gamelib_color_t color)
{
    int idx = ui_add(g, x, y, w, h, UI_TOGGLE);
    if (idx < 0) return false;
    ui_elems[idx].data.bval = value;

    if (*value) {
        gamelib_fill_rect(g, x, y, w, h, COLOR_DARKGRAY);
        gamelib_draw_rect(g, x + 1, y + 1, w - 2, h - 2, COLOR_GRAY);
    } else {
        gamelib_fill_rect(g, x, y, w, h, color);
        gamelib_draw_rect(g, x, y, w, h, COLOR_WHITE);
    }

    int lbl_w = (int)strlen(label) * 8;
    int tx = x + (w - lbl_w) / 2;
    int ty = y + (h - 8) / 2;
    gamelib_color_t tc = *value ? COLOR_LIGHTGRAY : COLOR_WHITE;
    gamelib_draw_text(g, tx, ty, label, tc);

    if (ui_activated(g, idx)) {
        *value = !*value;
        return true;
    }
    return false;
}
```

---

### Task 2: gamelib.h + gamelib.c 集成

**Files:**
- Modify: `components/gamelib_core/gamelib.h`
- Modify: `components/gamelib_core/gamelib.c`

#### gamelib.h

在 `#include "font.h"` 之后添加：

```c
#include "ui.h"
```

在 `/* --- helpers --- */` 之前添加新 API 声明：

```c
/* --- ui controls --- */
void gamelib_ui_begin(gamelib_t *g);
void gamelib_ui_end(gamelib_t *g);
bool gamelib_button(gamelib_t *g, int x, int y, int w, int h, const char *label, gamelib_color_t color);
bool gamelib_checkbox(gamelib_t *g, int x, int y, const char *label, bool *value);
bool gamelib_radio_box(gamelib_t *g, int x, int y, const char *label, int *value, int group_value);
bool gamelib_toggle_button(gamelib_t *g, int x, int y, int w, int h, const char *label, bool *value, gamelib_color_t color);
```

---

### Task 3: main.c — UI 演示

**Files:**
- Modify: `main/main.c`

纯 UI 演示（去掉所有 sprite/color/tilemap 代码）：

```c
#include <stdio.h>
#include "gamelib.h"

void gamelib_port_register_hal(void);
static gamelib_t game;

void app_main(void)
{
    gamelib_port_register_hal();
    if (gamelib_init(&game, 240, 320, 60) != 0) {
        printf("GameLib init failed\n");
        return;
    }

    bool music_on = true, sfx_on = true, grid_on = false, hard_mode = false;
    int  difficulty = 0;
    bool paused = false, turbo = false;
    int  start_n = 0, reset_n = 0;

    while (!gamelib_is_closed(&game)) {
        gamelib_clear(&game, COLOR_RGB(18, 22, 34));

        /* Title bar */
        gamelib_fill_rect(&game, 0, 0, 240, 40, COLOR_RGB(10, 14, 24));
        gamelib_draw_text(&game, 8, 8, "UI CONTROLS v5", COLOR_WHITE);
        gamelib_draw_text(&game, 8, 24, "D-Pad:nav  A:click", COLOR_LIGHTGRAY);

        gamelib_ui_begin(&game);

        /* Buttons */
        gamelib_draw_text(&game, 8, 50, "Buttons:", COLOR_WHITE);
        if (gamelib_button(&game, 8, 68, 100, 24, "START", COLOR_RGB(52, 150, 92))) start_n++;
        if (gamelib_button(&game, 120, 68, 100, 24, "RESET", COLOR_RGB(196, 142, 46))) {
            music_on = true; sfx_on = true; grid_on = false; hard_mode = false; reset_n++;
        }

        /* Checkboxes */
        gamelib_draw_text(&game, 8, 108, "Checkboxes:", COLOR_WHITE);
        gamelib_checkbox(&game, 8, 128, "MUSIC", &music_on);
        gamelib_checkbox(&game, 8, 148, "SFX", &sfx_on);
        gamelib_checkbox(&game, 8, 168, "GRID", &grid_on);
        gamelib_checkbox(&game, 8, 188, "HARD MODE", &hard_mode);

        /* Radio boxes */
        gamelib_draw_text(&game, 8, 220, "Difficulty:", COLOR_WHITE);
        gamelib_radio_box(&game, 8, 238, "EASY", &difficulty, 0);
        gamelib_radio_box(&game, 80, 238, "MEDIUM", &difficulty, 1);
        gamelib_radio_box(&game, 160, 238, "HARD", &difficulty, 2);

        /* Toggle buttons */
        gamelib_draw_text(&game, 8, 268, "Toggles:", COLOR_WHITE);
        gamelib_toggle_button(&game, 8, 286, 100, 24, "PAUSE", &paused, COLOR_RGB(180, 76, 76));
        gamelib_toggle_button(&game, 120, 286, 100, 24, "TURBO", &turbo, COLOR_RGB(52, 150, 92));

        /* Status */
        const char *diff_names[] = {"EASY", "MEDIUM", "HARD"};
        gamelib_draw_printf(&game, 5, 310, COLOR_LIGHTGRAY, "S:%d R:%d DIFF:%s",
                           start_n, reset_n, diff_names[difficulty]);
        gamelib_draw_printf(&game, 5, 318, COLOR_LIGHTGRAY, "PAUSE:%s TURBO:%s",
                           paused ? "Y" : "N", turbo ? "Y" : "N");

        gamelib_ui_end(&game);
        gamelib_update(&game);
        gamelib_wait_frame(&game);
    }
    gamelib_deinit(&game);
}
```

---

### 变更总结

| 文件 | 操作 | 行数 |
|------|------|------|
| `ui.h` | 新建 | +20 |
| `ui.c` | 新建 | ~170 |
| `gamelib.h` | 修改 | +7 |
| `main.c` | 重写 | ~70 |
