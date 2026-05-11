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
    (void)g;
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

    if (ui_focus < 0 || ui_focus >= ui_count) ui_focus = 0;

    if (gamelib_is_key_pressed(g, KEY_DOWN))
        ui_focus = (ui_focus + 1) % ui_count;
    if (gamelib_is_key_pressed(g, KEY_UP))
        ui_focus = (ui_focus - 1 + ui_count) % ui_count;
    if (gamelib_is_key_pressed(g, KEY_RIGHT))
        ui_focus = (ui_focus + 1) % ui_count;
    if (gamelib_is_key_pressed(g, KEY_LEFT))
        ui_focus = (ui_focus - 1 + ui_count) % ui_count;

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
    return (idx == ui_focus && gamelib_is_key_pressed(g, KEY_A));
}

bool gamelib_button(gamelib_t *g, int x, int y, int w, int h, const char *label, gamelib_color_t color)
{
    int idx = ui_add(g, x, y, w, h, UI_BUTTON);
    if (idx < 0) return false;

    gamelib_fill_rect(g, x, y, w, h, color);
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
