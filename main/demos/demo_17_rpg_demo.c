#include "gamelib.h"
#include "demo_17_rpg_demo_resource/tiled_project/map_data.c"
#include "demo_17_rpg_demo_resource/tiled_project/map_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 240
#define H 320
#define FRAME_W 96
#define FRAME_H 96

#define TILE_ZOOM 1
#define TS (MAP_TILE_WIDTH * TILE_ZOOM)  /* 48px screen tile size */

#define DST_FRAME_W (TS * 3)   /* player ~3 tiles tall */
#define DST_FRAME_H (TS * 3)

#include "esp_heap_caps.h"

static uint8_t* load_file(gamelib_t *g, const char *path, size_t *out_len) {
    void *f = gamelib_file_open(g, path, "rb");
    if (!f) return NULL;
    int size = gamelib_file_size(g, f);
    if (size <= 0) { gamelib_file_close(g, f); return NULL; }
    uint8_t *buf = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = (uint8_t*)malloc(size);
    if (!buf) { gamelib_file_close(g, f); return NULL; }
    int read = gamelib_file_read(g, f, buf, size);
    gamelib_file_close(g, f);
    if (read != size) { free(buf); return NULL; }
    if (out_len) *out_len = (size_t)size;
    return buf;
}

/* ====== 方向 / 玩家状态 / Slime状态 ====== */
typedef enum { DIR_DOWN, DIR_RIGHT, DIR_UP } dir_t;
typedef enum { PSTATE_IDLE, PSTATE_WALK, PSTATE_ATTACK, PSTATE_DEATH } pstate_t;
typedef enum { SSTATE_IDLE, SSTATE_MOVE, SSTATE_ATTACK, SSTATE_HURT, SSTATE_DEATH } sstate_t;

/* ====== 玩家精灵布局 (player.png, 48×48/帧) ====== */
static const int player_atlas[][4] = {
    /* IDLE  WALK  ATTACK  DEATH */
    { 0,     3,    6,      9 },   /* DIR_DOWN  */
    { 1,     4,    7,      9 },   /* DIR_RIGHT */
    { 2,     5,    8,      9 },   /* DIR_UP    */
};
static const int player_frames[] = { 6, 6, 4, 3 };

#define PANIM_IDLE   5
#define PANIM_WALK   3
#define PANIM_ATTACK 4
#define PANIM_DEATH  6
#define PLAYER_SPEED 3

/* ====== Slime精灵布局 (slime.png, 32×32/帧) ====== */
#define SLIME_FW 64
#define SLIME_FH 64
#define SLIME_DST (SLIME_FW * TILE_ZOOM)  /* 64px 屏幕尺寸 */

static const int slime_atlas[][5] = {
    /* IDLE  MOVE  ATTACK  HURT  DEATH */
    { 0,     3,    6,      9,    12 },  /* DIR_DOWN  */
    { 1,     4,    7,     10,    12 },  /* DIR_RIGHT */
    { 2,     5,    8,     11,    12 },  /* DIR_UP    */
};
static const int slime_frames[] = { 4, 6, 7, 4, 5 };

#define SANIM_IDLE   10
#define SANIM_MOVE    8
#define SANIM_ATTACK  6
#define SANIM_HURT    6
#define SANIM_DEATH   10

#define SLIME_HP         3
#define SLIME_PATROL_SPD  1
#define SLIME_DETECT_R   80   /* 发现玩家距离 (地图像素) */
#define SLIME_ATK_R      28   /* 攻击命中距离 (地图像素) */
#define PLAYER_ATK_R     40   /* 玩家攻击范围 (地图像素) */
#define PLAYER_MAX_HP     5
#define PLAYER_HURT_TIME 90   /* 受伤无敌帧数 */

typedef struct {
    float x, y;
    int   spawn_cx, spawn_cy;  /* 巡逻矩形中心 */
    int   patrol_hw, patrol_hh; /* 巡逻半宽/半高 */
    dir_t dir;
    bool  flip;          /* 水平翻转 (面朝左) */
    sstate_t state;
    int   frame, timer;
    int   hp;
    int   move_dir;      /* 当前巡逻方向: 1 或 -1 */
    int   move_timer;    /* 方向切换计时 */
    bool  alive;
} slime_t;

static dir_t vec_to_dir(float dx, float dy)
{
    if (dx > 0) return DIR_RIGHT;
    if (dx < 0) return DIR_RIGHT;  /* + flip */
    if (dy < 0) return DIR_UP;
    return DIR_DOWN;
}

static bool tile_is_obstacle(int gid)
{
    return gid >= 0 && gid < MAP_TILESET_TILEMAP_TILECOUNT && TILE_HAS_OBSTACLE(gid);
}

static bool map_is_blocked_at_pixel(int world_x, int world_y)
{
    if (world_x < 0 || world_x >= MAP_WIDTH * MAP_TILE_WIDTH ||
        world_y < 0 || world_y >= MAP_HEIGHT * MAP_TILE_HEIGHT) {
        return true;
    }

    int c = world_x / MAP_TILE_WIDTH;
    int r = world_y / MAP_TILE_HEIGHT;
    int i = r * MAP_WIDTH + c;

    return tile_is_obstacle(map_layer_layer_0_base[i]) ||
           tile_is_obstacle(map_layer_layer_1[i]);
}

static bool player_hits_obstacle(int center_x, int center_y)
{
    int body_hw = MAP_TILE_WIDTH / 4;
    int body_hh = MAP_TILE_HEIGHT / 4;

    return map_is_blocked_at_pixel(center_x - body_hw, center_y - body_hh) ||
           map_is_blocked_at_pixel(center_x + body_hw, center_y - body_hh) ||
           map_is_blocked_at_pixel(center_x - body_hw, center_y + body_hh) ||
           map_is_blocked_at_pixel(center_x + body_hw, center_y + body_hh);
}

static void demo_17_rpg_demo(gamelib_t *g) {
    /* === 加载 player.png === */
    int player_spr = -1;
    {
        size_t len; uint8_t *buf = load_file(g, "assert/player_2x.png", &len);
        if (buf) { player_spr = gamelib_sprite_load_png(g, buf, len); free(buf); }
    }
    if (player_spr < 0) {
        while (!gamelib_is_closed(g)) {
            if (gamelib_is_key_pressed(g, KEY_SELECT)) break;
            gamelib_clear(g, COLOR_BLACK);
            gamelib_draw_printf(g, 10, H/2-10, COLOR_RED, "player_2x.png not found");
            gamelib_update(g); gamelib_wait_frame(g);
        }
        return;
    }

    /* === 加载 slime.png === */
    int slime_spr = -1;
    {
        size_t len; uint8_t *buf = load_file(g, "assert/slime_2x.png", &len);
        if (buf) { slime_spr = gamelib_sprite_load_png(g, buf, len); free(buf); }
    }

    /* === 加载 tileset === */
    int tileset_id = -1;
    {
        size_t len; uint8_t *buf = load_file(g, "assert/tilemap_2x.png", &len);
        if (buf) { tileset_id = gamelib_sprite_load_png(g, buf, len); free(buf); }
    }

    /* === 创建 tilemap === */
    int map_id = gamelib_tilemap_create(g, MAP_WIDTH, MAP_HEIGHT,
                                         MAP_TILE_WIDTH, tileset_id);
    if (map_id >= 0) {
        for (int r = 0; r < MAP_HEIGHT; r++)
            for (int c = 0; c < MAP_WIDTH; c++)
                gamelib_tilemap_set_tile(g, map_id, c, r,
                    map_layer_layer_0_base[r * MAP_WIDTH + c]);
    }

    /* === 从 Tiled 对象层创建 Slime === */
    slime_t slimes[MAP_OBJ_SLIME_COUNT];
    int slime_count = MAP_OBJ_SLIME_COUNT;
    for (int i = 0; i < slime_count; i++) {
        slimes[i].x     = map_obj_slime[i].x;
        slimes[i].y     = map_obj_slime[i].y;
        slimes[i].spawn_cx = (int)(map_obj_slime[i].x);
        slimes[i].spawn_cy = (int)(map_obj_slime[i].y);
        slimes[i].patrol_hw = (int)(map_obj_slime[i].w) / 2;
        slimes[i].patrol_hh = (int)(map_obj_slime[i].h) / 2;
        slimes[i].dir   = DIR_DOWN;
        slimes[i].flip  = false;
        slimes[i].state = SSTATE_IDLE;
        slimes[i].frame = 0;
        slimes[i].timer = 0;
        slimes[i].hp    = SLIME_HP;
        slimes[i].move_dir = 1;
        slimes[i].move_timer = 0;
        slimes[i].alive = true;
    }

    /* === 玩家状态 === */
    int p_x = 7 * MAP_TILE_WIDTH + MAP_TILE_WIDTH / 2;
    int p_y = 10 * MAP_TILE_HEIGHT + MAP_TILE_HEIGHT / 2;
    dir_t    p_dir  = DIR_DOWN;
    bool     p_flip = false;
    pstate_t p_state = PSTATE_IDLE;
    int      p_frame = 0, p_timer = 0;
    bool     p_alive = true;
    int      p_hp   = PLAYER_MAX_HP;
    int      p_hurt_timer = 0;   /* 受伤后无敌计时 */

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_SELECT)) break;
        if (p_hurt_timer > 0) p_hurt_timer--;

        /* ========== 玩家输入 ========== */
        int mx = 0, my = 0;
        bool atk = false;
        if (p_alive) {
            if (gamelib_is_key_down(g, KEY_RIGHT)) { mx =  1; p_dir = DIR_RIGHT; p_flip = false; }
            if (gamelib_is_key_down(g, KEY_LEFT))  { mx = -1; p_dir = DIR_RIGHT; p_flip = true;  }
            if (gamelib_is_key_down(g, KEY_UP))    { my = -1; p_dir = DIR_UP;                     }
            if (gamelib_is_key_down(g, KEY_DOWN))  { my =  1; p_dir = DIR_DOWN;                   }
            atk = gamelib_is_key_pressed(g, KEY_A);
            if (atk && p_state != PSTATE_ATTACK && p_state != PSTATE_DEATH) {
                p_state = PSTATE_ATTACK; p_frame = 0; p_timer = 0;
            }
            // if (gamelib_is_key_pressed(g, KEY_B) && p_state != PSTATE_DEATH) {
            //     p_state = PSTATE_DEATH; p_frame = 0; p_timer = 0; p_alive = false;
            // }
        }
        bool p_moving = (mx != 0 || my != 0);

        /* 玩家状态 */
        if (p_state == PSTATE_ATTACK || p_state == PSTATE_DEATH) {
        } else if (p_moving) { p_state = PSTATE_WALK; }
        else                 { p_state = PSTATE_IDLE;  }

        /* 玩家移动 + 碰撞 */
        if (p_state != PSTATE_ATTACK && p_state != PSTATE_DEATH) {
            int nx = p_x + mx * PLAYER_SPEED;
            int ny = p_y + my * PLAYER_SPEED;

            /* 分轴碰撞，避免斜向移动时直接穿过 layer_1 上的障碍物 */
            if (!player_hits_obstacle(nx, p_y)) p_x = nx;
            if (!player_hits_obstacle(p_x, ny)) p_y = ny;
        } 

        /* 玩家动画 */
        {
            int spd = PANIM_IDLE, maxf = player_frames[p_state];
            if (p_state == PSTATE_WALK)   spd = PANIM_WALK;
            if (p_state == PSTATE_ATTACK) spd = PANIM_ATTACK;
            if (p_state == PSTATE_DEATH)  spd = PANIM_DEATH;
            p_timer++;
            if (p_timer >= spd) {
                p_timer = 0;
                if (p_state == PSTATE_ATTACK || p_state == PSTATE_DEATH) {
                    if (p_frame < maxf - 1) p_frame++;
                } else {
                    p_frame = (p_frame + 1) % maxf;
                }
            }
            if (p_state == PSTATE_ATTACK && p_frame >= maxf - 1 && p_timer >= spd / 2) {
                p_state = PSTATE_IDLE; p_frame = 0; p_timer = 0;
            }
        }

        /* ========== Slime AI ========== */
        for (int i = 0; i < slime_count; i++) {
            slime_t *s = &slimes[i];
            if (!s->alive) continue;

            /* 死亡状态: 只播动画 */
            if (s->state == SSTATE_DEATH) goto slime_anim;

            /* 受伤状态: 等待动画播完 */
            if (s->state == SSTATE_HURT) goto slime_anim;

            /* 计算到玩家距离 */
            float dx = (float)p_x - s->x;
            float dy = (float)p_y - s->y;
            float dist = sqrtf(dx * dx + dy * dy);

            /* AI 距离裁剪 (不在屏幕附近则不更新AI，也只播休息动画) */
            if (dist > W * 1.5f) {
                if (s->state != SSTATE_IDLE) { s->state = SSTATE_IDLE; s->frame = 0; s->timer = 0; }
                goto slime_anim;
            }

            if (dist < SLIME_ATK_R && s->state != SSTATE_ATTACK) {
                /* 近身 → 攻击: 向前扑一格(16px) */
                s->state = SSTATE_ATTACK;
                s->frame = 0; s->timer = 0;
                s->dir = vec_to_dir(dx, dy);
                s->flip = (dx < 0);
                if (dist > 1.0f) {
                    s->x += dx / dist * MAP_TILE_WIDTH;
                    s->y += dy / dist * MAP_TILE_WIDTH;
                }
            } else if (dist < SLIME_DETECT_R && s->state != SSTATE_ATTACK) {
                /* 发现玩家 → 追击 */
                s->state = SSTATE_MOVE;
                s->dir = vec_to_dir(dx, dy);
                s->flip = (dx < 0);
                if (dist > 1.0f) {
                    s->x += dx / dist * SLIME_PATROL_SPD * 2;
                    s->y += dy / dist * SLIME_PATROL_SPD * 2;
                }
            } else if (s->state != SSTATE_ATTACK) {
                /* 巡逻 */
                s->state = SSTATE_MOVE;
                s->move_timer++;
                if (s->move_timer > 60) { s->move_dir *= -1; s->move_timer = 0; }

                s->x += s->move_dir * SLIME_PATROL_SPD;
                /* 边界反弹 */
                if (s->x < s->spawn_cx - s->patrol_hw) { s->x = s->spawn_cx - s->patrol_hw; s->move_dir =  1; }
                if (s->x > s->spawn_cx + s->patrol_hw) { s->x = s->spawn_cx + s->patrol_hw; s->move_dir = -1; }
                s->dir = DIR_RIGHT;
                s->flip = (s->move_dir < 0);
            }

slime_anim:
            /* Slime 动画计时 */
            {
                int spd = SANIM_IDLE, maxf = slime_frames[s->state];
                if (s->state == SSTATE_MOVE)   spd = SANIM_MOVE;
                if (s->state == SSTATE_ATTACK) spd = SANIM_ATTACK;
                if (s->state == SSTATE_HURT)   spd = SANIM_HURT;
                if (s->state == SSTATE_DEATH)  spd = SANIM_DEATH;
                s->timer++;
                if (s->timer >= spd) {
                    s->timer = 0;
                    if (s->state == SSTATE_ATTACK || s->state == SSTATE_HURT || s->state == SSTATE_DEATH) {
                        if (s->frame < maxf - 1) s->frame++;
                    } else {
                        s->frame = (s->frame + 1) % maxf;
                    }
                }
                /* slime 攻击命中帧(第3帧): 检测是否击中玩家 */
                if (s->state == SSTATE_ATTACK && s->frame == 3 && s->timer == 0) {
                    float dx2 = (float)p_x - s->x;
                    float dy2 = (float)p_y - s->y;
                    if (sqrtf(dx2*dx2 + dy2*dy2) < SLIME_ATK_R * 2 && p_hurt_timer <= 0) {
                        p_hp--;
                        p_hurt_timer = PLAYER_HURT_TIME;
                        if (p_hp <= 0) {
                            p_state = PSTATE_DEATH; p_frame = 0; p_timer = 0; p_alive = false;
                        }
                    }
                }
                /* 受伤动画播完 → 回 IDLE */
                if (s->state == SSTATE_HURT && s->frame >= maxf - 1 && s->timer >= spd / 2) {
                    s->state = SSTATE_IDLE; s->frame = 0;
                }
                /* 攻击动画播完 → 回 IDLE */
                if (s->state == SSTATE_ATTACK && s->frame >= maxf - 1 && s->timer >= spd / 2) {
                    s->state = SSTATE_IDLE; s->frame = 0;
                }
                /* 死亡动画播完 → 消失 */
                if (s->state == SSTATE_DEATH && s->frame >= maxf - 1) {
                    s->alive = false;
                }
            }
        }

        /* ========== 玩家攻击命中检测 ========== */
        if (atk) {
            for (int i = 0; i < slime_count; i++) {
                slime_t *s = &slimes[i];
                if (!s->alive) continue;
                if (s->state == SSTATE_DEATH || s->state == SSTATE_HURT) continue;
                float dx = (float)p_x - s->x;
                float dy = (float)p_y - s->y;
                if (sqrtf(dx*dx + dy*dy) < PLAYER_ATK_R) {
                    s->hp--;
                    if (s->hp <= 0) {
                        s->state = SSTATE_DEATH; s->frame = 0; s->timer = 0;
                    } else {
                        s->state = SSTATE_HURT; s->frame = 0; s->timer = 0;
                    }
                    break;  /* 每次攻击只命中一个 */
                }
            }
        }

        /* ========== 摄像机 + 视口裁剪 ========== */
        int csx = p_x * TILE_ZOOM - W / 2;
        int csy = p_y * TILE_ZOOM - H / 2;
        int max_csx = MAP_WIDTH * TS - W;
        int max_csy = MAP_HEIGHT * TS - H;
        if (csx < 0) csx = 0;
        else if (csx > max_csx) csx = max_csx;
        if (csy < 0) csy = 0;
        else if (csy > max_csy) csy = max_csy;

        int sc0 = csx / TS;          if (sc0 < 0) sc0 = 0;
        int sr0 = csy / TS;          if (sr0 < 0) sr0 = 0;
        int sc1 = (csx + W + TS-1)/TS; if (sc1 > MAP_WIDTH)  sc1 = MAP_WIDTH;
        int sr1 = (csy + H + TS-1)/TS; if (sr1 > MAP_HEIGHT) sr1 = MAP_HEIGHT;

        /* ========== 渲染 ========== */

        /* 地图 */
        if (tileset_id >= 0) {
            int cols = MAP_TILESET_TILEMAP_COLUMNS;
            for (int r = sr0; r < sr1; r++) {
                for (int c = sc0; c < sc1; c++) {
                    int i = r * MAP_WIDTH + c;
                    int sx = c * TS - csx, sy = r * TS - csy;
                    int tid = map_layer_layer_0_base[i];
                    if (tid >= 0)
                        gamelib_draw_sprite_region(g, tileset_id, sx, sy,
                            (tid % cols) * MAP_TILE_WIDTH,
                            (tid / cols) * MAP_TILE_HEIGHT,
                            MAP_TILE_WIDTH, MAP_TILE_HEIGHT);
                    tid = map_layer_layer_1[i];
                    if (tid >= 0)
                        gamelib_draw_sprite_region(g, tileset_id, sx, sy,
                            (tid % cols) * MAP_TILE_WIDTH,
                            (tid / cols) * MAP_TILE_HEIGHT,
                            MAP_TILE_WIDTH, MAP_TILE_HEIGHT);
                }
            }
        }

        /* Slime */
        if (slime_spr >= 0) {
            for (int i = 0; i < slime_count; i++) {
                slime_t *s = &slimes[i];
                if (!s->alive) continue;
                int dx = (int)(s->x * TILE_ZOOM) - csx - SLIME_DST / 2;
                int dy = (int)(s->y * TILE_ZOOM) - csy - SLIME_DST / 2;
                if (dx + SLIME_DST < 0 || dx >= W || dy + SLIME_DST < 0 || dy >= H) continue;

                int row = slime_atlas[s->dir][s->state];
                int flg = SPRITE_COLORKEY | (s->flip ? SPRITE_FLIP_H : 0);
                gamelib_draw_sprite_atlas_frame_scaled(g, slime_spr, dx, dy,
                    0, row * SLIME_FH, SLIME_FW, SLIME_FH, s->frame,
                    SLIME_DST, SLIME_DST, flg);
            }
        }

        /* 玩家 */
        {
            int row = player_atlas[p_dir][p_state];
            int dx = p_x * TILE_ZOOM - csx - DST_FRAME_W / 2;
            int dy = p_y * TILE_ZOOM - csy - DST_FRAME_H / 2;
            if (p_hurt_timer % 4 < 2) { /* 闪烁效果 */
                int flg = SPRITE_COLORKEY | (p_flip ? SPRITE_FLIP_H : 0);
                gamelib_draw_sprite_atlas_frame_scaled(g, player_spr, dx, dy,
                    0, row * FRAME_H, FRAME_W, FRAME_H, p_frame,
                    DST_FRAME_W, DST_FRAME_H, flg);
            }
        }

        /* HUD */
        {
            /* 背景半透明/黑色底以防文字看不清，局部清除 */
            // gamelib_fill_rect(g, 0, 0, 140, 40, gamelib_color_alpha(COLOR_BLACK, 128));
            // gamelib_fill_rect(g, 0, H - 16, 240, 16, gamelib_color_alpha(COLOR_BLACK, 128));

            /* 玩家血条 */
            int bar_w = 80, bar_h = 6;
            gamelib_draw_rect(g, 5, 5, bar_w, bar_h, COLOR_WHITE);
            if (p_hp > 0)
                gamelib_fill_rect(g, 6, 6, (bar_w - 2) * p_hp / PLAYER_MAX_HP, bar_h - 2, COLOR_RED);
            gamelib_draw_printf(g, 5, 14, COLOR_WHITE, "HP:%d/%d", p_hp, PLAYER_MAX_HP);

            int alive_ct = 0;
            for (int i = 0; i < slime_count; i++) if (slimes[i].alive) alive_ct++;
            gamelib_draw_printf(g, 5, 26, COLOR_GRAY,
                "Tile:%d,%d  Enemies:%d",
                p_x / MAP_TILE_WIDTH, p_y / MAP_TILE_HEIGHT, alive_ct);
            gamelib_draw_printf(g, 5, H - 12, COLOR_DARKGRAY,
                "FPS:%.0f DPAD:move A:atk SEL:quit", gamelib_get_fps(g));
        }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }

    if (map_id >= 0) gamelib_tilemap_free(g, map_id);
    gamelib_sprite_free(g, player_spr);
    if (slime_spr >= 0) gamelib_sprite_free(g, slime_spr);
}
