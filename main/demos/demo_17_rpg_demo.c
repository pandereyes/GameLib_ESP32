#include "gamelib.h"
#include "demo_17_rpg_demo_resource/tiled_project/map_data.c"
#include "demo_17_rpg_demo_resource/tiled_project/map_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "anim.h"
#include "collision.h"
#include "camera.h"

#define W 240
#define H 320
#define FRAME_W 96
#define FRAME_H 96

#define TILE_ZOOM 1
#define TS (MAP_TILE_WIDTH * TILE_ZOOM)  /* 48px screen tile size */

#define DST_FRAME_W (TS * 3)   /* player ~3 tiles tall */
#define DST_FRAME_H (TS * 3)

#define DEMO17_SHOW_COLLISION_DEBUG 1

/* ====== 方向 / 玩家状态 / Slime状态 ====== */
typedef enum { DIR_DOWN, DIR_RIGHT, DIR_UP } dir_t;
typedef enum { PSTATE_IDLE, PSTATE_WALK, PSTATE_ATTACK, PSTATE_DEATH } pstate_t;
typedef enum { SSTATE_IDLE, SSTATE_MOVE, SSTATE_ATTACK, SSTATE_HURT, SSTATE_DEATH } sstate_t;

/* ====== 玩家精灵布局 (player.png, 48×48/帧) ====== */
/* player animation frame duration in milliseconds */
#define PANIM_IDLE_MS    83
#define PANIM_WALK_MS    50
#define PANIM_ATTACK_MS  67
#define PANIM_DEATH_MS  100
#define PLAYER_SPEED 3

/* ====== Slime精灵布局 (slime.png, 32×32/帧) ====== */
#define SLIME_FW 64
#define SLIME_FH 64
#define SLIME_DST (SLIME_FW * TILE_ZOOM)  /* 64px 屏幕尺寸 */

/* slime animation frame duration in milliseconds */
#define SANIM_IDLE_MS    167
#define SANIM_MOVE_MS    133
#define SANIM_ATTACK_MS  100
#define SANIM_HURT_MS    100
#define SANIM_DEATH_MS   167

#define SLIME_HP         3
#define SLIME_PATROL_SPD  1
#define SLIME_PATROL_TURN_MS 1000
#define PLAYER_HURT_TIME_MS 1500
#define SLIME_DETECT_R   80   /* 发现玩家距离 (地图像素) */
#define SLIME_ATK_R      28   /* 攻击命中距离 (地图像素) */
#define PLAYER_ATK_R     40   /* 玩家攻击范围 (地图像素) */
#define PLAYER_MAX_HP     5

static const gamelib_anim_clip_t player_clips[3][4] = {
    {   // DIR_DOWN
        {0, 0 * FRAME_H, FRAME_W, FRAME_H, 6, PANIM_IDLE_MS,   GAMELIB_ANIM_LOOP},
        {0, 3 * FRAME_H, FRAME_W, FRAME_H, 6, PANIM_WALK_MS,   GAMELIB_ANIM_LOOP},
        {0, 6 * FRAME_H, FRAME_W, FRAME_H, 4, PANIM_ATTACK_MS, GAMELIB_ANIM_ONCE},
        {0, 9 * FRAME_H, FRAME_W, FRAME_H, 3, PANIM_DEATH_MS,  GAMELIB_ANIM_HOLD_LAST},
    },
    {
        // DIR_RIGHT
        {0, 1 * FRAME_H, FRAME_W, FRAME_H, 6, PANIM_IDLE_MS,   GAMELIB_ANIM_LOOP},
        {0, 4 * FRAME_H, FRAME_W, FRAME_H, 6, PANIM_WALK_MS,   GAMELIB_ANIM_LOOP},
        {0, 7 * FRAME_H, FRAME_W, FRAME_H, 4, PANIM_ATTACK_MS, GAMELIB_ANIM_ONCE},
        {0, 9 * FRAME_H, FRAME_W, FRAME_H, 3, PANIM_DEATH_MS,  GAMELIB_ANIM_HOLD_LAST},
    },
    {
        // DIR_UP
        {0, 2 * FRAME_H, FRAME_W, FRAME_H, 6, PANIM_IDLE_MS,   GAMELIB_ANIM_LOOP},
        {0, 5 * FRAME_H, FRAME_W, FRAME_H, 6, PANIM_WALK_MS,   GAMELIB_ANIM_LOOP},
        {0, 8 * FRAME_H, FRAME_W, FRAME_H, 4, PANIM_ATTACK_MS, GAMELIB_ANIM_ONCE},
        {0, 9 * FRAME_H, FRAME_W, FRAME_H, 3, PANIM_DEATH_MS,  GAMELIB_ANIM_HOLD_LAST},
    },
};

static const gamelib_anim_clip_t slime_clips[3][5] = {
    {
        {0, 0 * SLIME_FH,  SLIME_FW, SLIME_FH, 4, SANIM_IDLE_MS,   GAMELIB_ANIM_LOOP},
        {0, 3 * SLIME_FH,  SLIME_FW, SLIME_FH, 6, SANIM_MOVE_MS,   GAMELIB_ANIM_LOOP},
        {0, 6 * SLIME_FH,  SLIME_FW, SLIME_FH, 7, SANIM_ATTACK_MS, GAMELIB_ANIM_ONCE},
        {0, 9 * SLIME_FH,  SLIME_FW, SLIME_FH, 4, SANIM_HURT_MS,   GAMELIB_ANIM_ONCE},
        {0, 12 * SLIME_FH, SLIME_FW, SLIME_FH, 5, SANIM_DEATH_MS,  GAMELIB_ANIM_HOLD_LAST},
    },
    {
        {0, 1 * SLIME_FH,  SLIME_FW, SLIME_FH, 4, SANIM_IDLE_MS,   GAMELIB_ANIM_LOOP},
        {0, 4 * SLIME_FH,  SLIME_FW, SLIME_FH, 6, SANIM_MOVE_MS,   GAMELIB_ANIM_LOOP},
        {0, 7 * SLIME_FH,  SLIME_FW, SLIME_FH, 7, SANIM_ATTACK_MS, GAMELIB_ANIM_ONCE},
        {0, 10 * SLIME_FH, SLIME_FW, SLIME_FH, 4, SANIM_HURT_MS,   GAMELIB_ANIM_ONCE},
        {0, 12 * SLIME_FH, SLIME_FW, SLIME_FH, 5, SANIM_DEATH_MS,  GAMELIB_ANIM_HOLD_LAST},
    },
    {
        {0, 2 * SLIME_FH,  SLIME_FW, SLIME_FH, 4, SANIM_IDLE_MS,   GAMELIB_ANIM_LOOP},
        {0, 5 * SLIME_FH,  SLIME_FW, SLIME_FH, 6, SANIM_MOVE_MS,   GAMELIB_ANIM_LOOP},
        {0, 8 * SLIME_FH,  SLIME_FW, SLIME_FH, 7, SANIM_ATTACK_MS, GAMELIB_ANIM_ONCE},
        {0, 11 * SLIME_FH, SLIME_FW, SLIME_FH, 4, SANIM_HURT_MS,   GAMELIB_ANIM_ONCE},
        {0, 12 * SLIME_FH, SLIME_FW, SLIME_FH, 5, SANIM_DEATH_MS,  GAMELIB_ANIM_HOLD_LAST},
    },
};

typedef struct {
    float x, y;
    int   spawn_cx, spawn_cy;  /* 巡逻矩形中心 */
    int   patrol_hw, patrol_hh; /* 巡逻半宽/半高 */
    dir_t dir;
    bool  flip;          /* 水平翻转 (面朝左) */
    sstate_t state;
    gamelib_animator_t anim;
    int   hp;
    uint32_t move_elapsed_ms;
    int   move_dir;      /* 当前巡逻方向: 1 或 -1 */
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

static bool tile_is_obstacle_cb(int tile_id, void *user)
{
    (void)user;
    return tile_is_obstacle(tile_id);
}

static bool player_hits_obstacle(const gamelib_collision_map_t *collision, int center_x, int center_y)
{
    int body_hw = MAP_TILE_WIDTH / 4;
    int body_hh = MAP_TILE_HEIGHT / 4;
    int torso_y = center_y + MAP_TILE_HEIGHT / 2; 

    return gamelib_collision_map_is_rect_blocked(collision,
        center_x - body_hw, torso_y - body_hh, body_hw * 2 + 1, body_hh * 2 + 1);
}

static void slime_set_state(slime_t *s, sstate_t state)
{
    if (!s) return;
    if (s->state != state) {
        s->state = state;
        gamelib_animator_play(&s->anim, &slime_clips[s->dir][s->state]);
    } else {
        gamelib_animator_set_clip(&s->anim, &slime_clips[s->dir][s->state]);
    }
}

static void player_set_state(gamelib_animator_t *anim, dir_t dir,
                             pstate_t *state, pstate_t next_state)
{
    if (!anim || !state) return;
    if (*state != next_state) {
        *state = next_state;
        gamelib_animator_play(anim, &player_clips[dir][*state]);
    } else {
        gamelib_animator_set_clip(anim, &player_clips[dir][*state]);
    }
}

static void slime_spawn_from_obj(slime_t *s, const map_obj_t *obj)
{
    if (!s || !obj) return;

    float center_x = obj->x + obj->w * 0.5f;
    float center_y = obj->y + obj->h * 0.5f;

    s->x = center_x;
    s->y = center_y;
    s->spawn_cx = (int)center_x;
    s->spawn_cy = (int)center_y;
    s->patrol_hw = (int)(obj->w * 0.5f);
    s->patrol_hh = (int)(obj->h * 0.5f);
}

static void demo_17_rpg_demo(gamelib_t *g) {
    /* === 加载 player.png === */
    int player_spr = gamelib_sprite_load_png_file(g, "assert/player_2x.png");
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
    int slime_spr = gamelib_sprite_load_png_file(g, "assert/slime_2x.png");

    /* === 加载 tileset === */
    int tileset_id = gamelib_sprite_load_png_file(g, "assert/tilemap_2x.png");

    /* === 创建 tilemap === */
    int map_id = gamelib_tilemap_create(g, MAP_WIDTH, MAP_HEIGHT,
                                         MAP_TILE_WIDTH, tileset_id);
    if (map_id >= 0) {
        for (int r = 0; r < MAP_HEIGHT; r++)
            for (int c = 0; c < MAP_WIDTH; c++)
                gamelib_tilemap_set_tile(g, map_id, c, r,
                    map_layer_layer_0_base[r * MAP_WIDTH + c]);
    }

    gamelib_collision_map_t collision;
    gamelib_collision_map_init(&collision, MAP_WIDTH, MAP_HEIGHT,
                               MAP_TILE_WIDTH, MAP_TILE_HEIGHT,
                               tile_is_obstacle_cb, NULL);
    gamelib_collision_map_add_layer(&collision, map_layer_layer_0_base);
    gamelib_collision_map_add_layer(&collision, map_layer_layer_1);

    gamelib_camera_t camera;
    gamelib_camera_init(&camera, W, H, MAP_WIDTH * TS, MAP_HEIGHT * TS);

    /* === 从 Tiled 对象层创建 Slime === */
    slime_t slimes[MAP_OBJ_SLIME_COUNT];
    int slime_count = MAP_OBJ_SLIME_COUNT;
    for (int i = 0; i < slime_count; i++) {
        slime_spawn_from_obj(&slimes[i], &map_obj_slime[i]);
        slimes[i].dir   = DIR_DOWN;
        slimes[i].flip  = false;
        slimes[i].state = SSTATE_IDLE;
        gamelib_animator_init(&slimes[i].anim);
        gamelib_animator_play(&slimes[i].anim, &slime_clips[slimes[i].dir][slimes[i].state]);
        slimes[i].hp    = SLIME_HP;
        slimes[i].move_dir = 1;
        slimes[i].move_elapsed_ms = 0;
        slimes[i].alive = true;
    }

    /* === 玩家状态 === */
    int p_x = 7 * MAP_TILE_WIDTH + MAP_TILE_WIDTH / 2;
    int p_y = 10 * MAP_TILE_HEIGHT + MAP_TILE_HEIGHT / 2;
    dir_t    p_dir  = DIR_DOWN;
    bool     p_flip = false;
    pstate_t p_state = PSTATE_IDLE;
    gamelib_animator_t p_anim;
    gamelib_animator_init(&p_anim);
    gamelib_animator_play(&p_anim, &player_clips[p_dir][p_state]);
    bool     p_alive = true;
    int      p_hp   = PLAYER_MAX_HP;
    uint32_t p_hurt_ms = 0;
    uint32_t last_loop_ms = (uint32_t)(gamelib_get_time(g) * 1000.0);

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_SELECT)) break;
        uint32_t now_ms = (uint32_t)(gamelib_get_time(g) * 1000.0);
        uint32_t dt_ms = now_ms - last_loop_ms;
        last_loop_ms = now_ms;
        if (dt_ms > 100) dt_ms = 100;
        if (p_hurt_ms > dt_ms) {
            p_hurt_ms -= dt_ms;
        } else {
            p_hurt_ms = 0;
        }
        uint32_t p_blink_phase = p_hurt_ms / 67U;

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
                player_set_state(&p_anim, p_dir, &p_state, PSTATE_ATTACK);
            }
            // if (gamelib_is_key_pressed(g, KEY_B) && p_state != PSTATE_DEATH) {
            //     player_set_state(&p_anim, p_dir, &p_state, PSTATE_DEATH); p_alive = false;
            // }
        }
        bool p_moving = (mx != 0 || my != 0);

        /* 玩家状态 */
        if (p_state == PSTATE_ATTACK || p_state == PSTATE_DEATH) {
            gamelib_animator_set_clip(&p_anim, &player_clips[p_dir][p_state]);
        } else if (p_moving) { player_set_state(&p_anim, p_dir, &p_state, PSTATE_WALK); }
        else                 { player_set_state(&p_anim, p_dir, &p_state, PSTATE_IDLE);  }

        /* 玩家移动 + 碰撞 */
        if (p_state != PSTATE_ATTACK && p_state != PSTATE_DEATH) {
            int nx = p_x + mx * PLAYER_SPEED;
            int ny = p_y + my * PLAYER_SPEED;

            /* 分轴碰撞，避免斜向移动时直接穿过 layer_1 上的障碍物 */
            if (!player_hits_obstacle(&collision, nx, p_y)) p_x = nx;
            if (!player_hits_obstacle(&collision, p_x, ny)) p_y = ny;
        } 

        /* 玩家动画 */
        {
            gamelib_animator_update(g, &p_anim);
            if (p_state == PSTATE_ATTACK && gamelib_animator_finished(&p_anim)) {
                player_set_state(&p_anim, p_dir, &p_state, PSTATE_IDLE);
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

            /* 计算到玩家躯干的距离 */
            float dx = (float)p_x - s->x;
            float dy = ((float)p_y + MAP_TILE_HEIGHT / 2.0f) - s->y;
            float dist = sqrtf(dx * dx + dy * dy);

            /* AI 距离裁剪 (不在屏幕附近则不更新AI，也只播休息动画) */
            if (dist > W * 1.5f) {
                slime_set_state(s, SSTATE_IDLE);
                goto slime_anim;
            }

            if (dist < SLIME_ATK_R && s->state != SSTATE_ATTACK) {
                /* 近身 → 攻击: 向前扑一格(16px) */
                s->dir = vec_to_dir(dx, dy);
                s->flip = (dx < 0);
                slime_set_state(s, SSTATE_ATTACK);
                if (dist > 1.0f) {
                    s->x += dx / dist * MAP_TILE_WIDTH;
                    s->y += dy / dist * MAP_TILE_WIDTH;
                }
            } else if (dist < SLIME_DETECT_R && s->state != SSTATE_ATTACK) {
                /* 发现玩家 → 追击 */
                s->dir = vec_to_dir(dx, dy);
                s->flip = (dx < 0);
                slime_set_state(s, SSTATE_MOVE);
                if (dist > 1.0f) {
                    s->x += dx / dist * SLIME_PATROL_SPD * 2;
                    s->y += dy / dist * SLIME_PATROL_SPD * 2;
                }
            } else if (s->state != SSTATE_ATTACK) {
                /* 巡逻 */
                s->move_elapsed_ms += dt_ms;
                if (s->move_elapsed_ms >= SLIME_PATROL_TURN_MS) {
                    s->move_dir *= -1;
                    s->move_elapsed_ms = 0;
                }

                s->x += s->move_dir * SLIME_PATROL_SPD;
                /* 边界反弹 */
                if (s->x < s->spawn_cx - s->patrol_hw) { s->x = s->spawn_cx - s->patrol_hw; s->move_dir =  1; }
                if (s->x > s->spawn_cx + s->patrol_hw) { s->x = s->spawn_cx + s->patrol_hw; s->move_dir = -1; }
                s->dir = DIR_RIGHT;
                s->flip = (s->move_dir < 0);
                slime_set_state(s, SSTATE_MOVE);
            }

slime_anim:
            /* Slime 动画计时 */
            {
                gamelib_animator_update(g, &s->anim);
                /* slime 攻击命中帧(第3帧): 检测是否击中玩家 */
                if (s->state == SSTATE_ATTACK &&
                    gamelib_animator_frame(&s->anim) == 3 &&
                    gamelib_animator_frame_changed(&s->anim)) {
                    float dx2 = (float)p_x - s->x;
                    float dy2 = ((float)p_y + MAP_TILE_HEIGHT / 2.0f) - s->y;
                    if (sqrtf(dx2*dx2 + dy2*dy2) < SLIME_ATK_R * 2 && p_hurt_ms == 0) {
                        p_hp--;
                        p_hurt_ms = PLAYER_HURT_TIME_MS;
                        if (p_hp <= 0) {
                            player_set_state(&p_anim, p_dir, &p_state, PSTATE_DEATH);
                            p_alive = false;
                        }
                    }
                }
                /* 受伤动画播完 → 回 IDLE */
                if (s->state == SSTATE_HURT && gamelib_animator_finished(&s->anim)) {
                    slime_set_state(s, SSTATE_IDLE);
                }
                /* 攻击动画播完 → 回 IDLE */
                if (s->state == SSTATE_ATTACK && gamelib_animator_finished(&s->anim)) {
                    slime_set_state(s, SSTATE_IDLE);
                }
                /* 死亡动画播完 → 消失 */
                if (s->state == SSTATE_DEATH && gamelib_animator_finished(&s->anim)) {
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
                float dy = ((float)p_y + MAP_TILE_HEIGHT / 2.0f) - s->y;
                if (sqrtf(dx*dx + dy*dy) < PLAYER_ATK_R) {
                    s->hp--;
                    if (s->hp <= 0) {
                        slime_set_state(s, SSTATE_DEATH);
                    } else {
                        slime_set_state(s, SSTATE_HURT);
                    }
                }
            }
        }

        /* ========== 摄像机 + 视口裁剪 ========== */
        gamelib_camera_follow(&camera, p_x * TILE_ZOOM, p_y * TILE_ZOOM);
        int csx = camera.x;
        int csy = camera.y;

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

        draw_collision_debug(g, csx, csy, sc0, sr0, sc1, sr1, p_x, p_y);

        /* Slime */
        if (slime_spr >= 0) {
            for (int i = 0; i < slime_count; i++) {
                slime_t *s = &slimes[i];
                if (!s->alive) continue;
                int dx = (int)(s->x * TILE_ZOOM) - csx - SLIME_DST / 2;
                int dy = (int)(s->y * TILE_ZOOM) - csy - SLIME_DST / 2;
                if (dx + SLIME_DST < 0 || dx >= W || dy + SLIME_DST < 0 || dy >= H) continue;

                const gamelib_anim_clip_t *clip = gamelib_animator_clip(&s->anim);
                if (!clip) continue;
                int flg = SPRITE_COLORKEY | (s->flip ? SPRITE_FLIP_H : 0);
                gamelib_draw_sprite_atlas_frame_scaled(g, slime_spr, dx, dy,
                    clip->atlas_x, clip->atlas_y, clip->frame_w, clip->frame_h,
                    gamelib_animator_frame(&s->anim),
                    SLIME_DST, SLIME_DST, flg);
            }
        }

        /* 玩家 */
        {
            const gamelib_anim_clip_t *clip = gamelib_animator_clip(&p_anim);
            int dx = p_x * TILE_ZOOM - csx - DST_FRAME_W / 2;
            int dy = p_y * TILE_ZOOM - csy - DST_FRAME_H / 2;
            if (clip && p_blink_phase % 4U < 2U) { /* 闪烁效果 */
                int flg = SPRITE_COLORKEY | (p_flip ? SPRITE_FLIP_H : 0);
                gamelib_draw_sprite_atlas_frame_scaled(g, player_spr, dx, dy,
                    clip->atlas_x, clip->atlas_y, clip->frame_w, clip->frame_h,
                    gamelib_animator_frame(&p_anim),
                    DST_FRAME_W, DST_FRAME_H, flg);
            }
        }

        /* HUD */
        {
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
                "FPS:%.0f", gamelib_get_fps(g));
        }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }

    if (map_id >= 0) gamelib_tilemap_free(g, map_id);
    gamelib_sprite_free(g, player_spr);
    if (slime_spr >= 0) gamelib_sprite_free(g, slime_spr);
    if (tileset_id >= 0) gamelib_sprite_free(g, tileset_id);
}
