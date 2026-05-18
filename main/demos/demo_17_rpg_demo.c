#include "gamelib.h"
#include "demo_17_rpg_demo_resource/tiled_project/map_data.c"
#include "demo_17_rpg_demo_resource/tiled_project/map_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 240
#define H 320
#define FRAME_W 48
#define FRAME_H 48

#define TILE_ZOOM 2
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

/* ====== 玩家动画状态机 ====== */
typedef enum { DIR_DOWN, DIR_RIGHT, DIR_UP } dir_t;
typedef enum { STATE_IDLE, STATE_WALK, STATE_ATTACK, STATE_DEATH } state_t;

/* player.png 中每一排的首帧在 atlas 中的行号 */
static const int atlas_row[][4] = {
    /* IDLE         WALK         ATTACK        DEATH */
    { 0,            3,           6,            9 },   /* DIR_DOWN  */
    { 1,            4,           7,            9 },   /* DIR_RIGHT */
    { 2,            5,           8,            9 },   /* DIR_UP    */
};
static const int frame_counts[] = { 6, 6, 4, 3 };  /* IDLE/WALK/ATTACK/DEATH */

#define ANIM_SPEED_IDLE    5
#define ANIM_SPEED_WALK    1
#define ANIM_SPEED_ATTACK  2
#define ANIM_SPEED_DEATH   6

#define PLAYER_SPEED_DEFAULT 3

static void demo_17_rpg_demo(gamelib_t *g) {
    /* --- 加载 player.png (单张含全部动画帧) --- */
    int player_spr = -1;
    {
        size_t len;
        uint8_t *buf = load_file(g, "assert/player/player.png", &len);
        if (buf) {
            player_spr = gamelib_sprite_load_png(g, buf, len);
            free(buf);
        }
    }
    if (player_spr < 0) {
        while (!gamelib_is_closed(g)) {
            if (gamelib_is_key_pressed(g, KEY_SELECT)) break;
            gamelib_clear(g, COLOR_BLACK);
            gamelib_draw_printf(g, 10, H / 2 - 10, COLOR_RED, "player.png not found");
            gamelib_draw_printf(g, 10, H / 2 + 10, COLOR_WHITE,
                                "Need assert/player/player.png");
            gamelib_update(g);
            gamelib_wait_frame(g);
        }
        return;
    }

    /* --- Tilemap setup --- */
    int tileset_id = -1;
    {
        size_t len;
        uint8_t *buf = load_file(g, "assert/tileset/tilemap.png", &len);
        if (buf) {
            tileset_id = gamelib_sprite_load_png(g, buf, len);
            free(buf);
        }
    }

    int map_id = gamelib_tilemap_create(g, MAP_WIDTH, MAP_HEIGHT,
                                         MAP_TILE_WIDTH, tileset_id);
    if (map_id >= 0) {
        for (int r = 0; r < MAP_HEIGHT; r++) {
            for (int c = 0; c < MAP_WIDTH; c++) {
                int idx = r * MAP_WIDTH + c;
                gamelib_tilemap_set_tile(g, map_id, c, r,
                                         map_layer_layer_0_base[idx]);
            }
        }
    }

    /* 玩家状态 */
    int player_x = 7 * MAP_TILE_WIDTH + MAP_TILE_WIDTH / 2;
    int player_y = 10 * MAP_TILE_HEIGHT + MAP_TILE_HEIGHT / 2;
    int player_speed = PLAYER_SPEED_DEFAULT;
    dir_t   player_dir   = DIR_DOWN;
    bool    player_flip  = false;   /* DIR_RIGHT + flip = 面朝左 */
    state_t player_state = STATE_IDLE;
    int     cur_frame    = 0;
    int     frame_timer  = 0;
    bool    alive        = true;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_SELECT)) break;

        /* === 输入处理: 方向 === */
        int move_x = 0, move_y = 0;
        if (gamelib_is_key_down(g, KEY_RIGHT)) { move_x =  1; player_dir = DIR_RIGHT; player_flip = false; }
        if (gamelib_is_key_down(g, KEY_LEFT))  { move_x = -1; player_dir = DIR_RIGHT; player_flip = true;  }
        if (gamelib_is_key_down(g, KEY_UP))    { move_y = -1; player_dir = DIR_UP;                     }
        if (gamelib_is_key_down(g, KEY_DOWN))  { move_y =  1; player_dir = DIR_DOWN;                   }
        bool moving = (move_x != 0 || move_y != 0);

        /* === 输入处理: 攻击 === */
        bool attack_pressed = gamelib_is_key_pressed(g, KEY_A);
        if (attack_pressed && alive &&
            player_state != STATE_ATTACK && player_state != STATE_DEATH) {
            player_state = STATE_ATTACK;
            cur_frame = 0;
            frame_timer = 0;
        }

        /* === 输入处理: 死亡 (KEY_B 测试用) === */
        if (gamelib_is_key_pressed(g, KEY_B) && alive && player_state != STATE_DEATH) {
            player_state = STATE_DEATH;
            cur_frame = 0;
            frame_timer = 0;
            alive = false;
        }

        /* === 状态逻辑 === */
        if (player_state == STATE_ATTACK || player_state == STATE_DEATH) {
            /* 攻击/死亡期间不移动, 只播动画 */
        } else if (moving) {
            player_state = STATE_WALK;
        } else {
            player_state = STATE_IDLE;
        }

        /* === 移动 + 碰撞 (攻击/死亡期间禁止移动) === */
        if (player_state != STATE_ATTACK && player_state != STATE_DEATH) {
            int new_x = player_x + move_x * player_speed;
            int new_y = player_y + move_y * player_speed;
            int map_w = MAP_WIDTH * MAP_TILE_WIDTH;
            int map_h = MAP_HEIGHT * MAP_TILE_HEIGHT;
            if (new_x >= 0 && new_x < map_w && new_y >= 0 && new_y < map_h) {
                int half = MAP_TILE_WIDTH / 2;
                int tile_gid = gamelib_tilemap_get_at_pixel(g, map_id,
                                                             new_x + half, new_y + half);
                if (tile_gid >= 0) {
                    if (!TILE_HAS_OBSTACLE(tile_gid)) {
                        player_x = new_x;
                        player_y = new_y;
                    }
                } else {
                    player_x = new_x;
                    player_y = new_y;
                }
            }
        }

        /* === 动画计时 === */
        int spd = ANIM_SPEED_IDLE;
        if (player_state == STATE_WALK)   spd = ANIM_SPEED_WALK;
        if (player_state == STATE_ATTACK) spd = ANIM_SPEED_ATTACK;
        if (player_state == STATE_DEATH)  spd = ANIM_SPEED_DEATH;
        int max_frames = frame_counts[player_state];

        frame_timer++;
        if (frame_timer >= spd) {
            frame_timer = 0;
            if (player_state == STATE_ATTACK || player_state == STATE_DEATH) {
                /* 一次性动画: 到最后一帧后停在最后一帧 */
                if (cur_frame < max_frames - 1)
                    cur_frame++;
                /* 攻击结束回到 IDLE */
                if (player_state == STATE_ATTACK && cur_frame >= max_frames - 1) {
                    /* 保持在最后一帧短暂时间, 下一轮自动切回 */
                }
            } else {
                cur_frame = (cur_frame + 1) % max_frames;
            }
        }

        /* 攻击动画播完后自动回到 IDLE */
        if (player_state == STATE_ATTACK && cur_frame >= max_frames - 1 && frame_timer >= spd / 2) {
            player_state = STATE_IDLE;
            cur_frame = 0;
            frame_timer = 0;
        }

        /* === 摄像机 === */
        int csx = player_x * TILE_ZOOM - W / 2;
        int csy = player_y * TILE_ZOOM - H / 2;

        /* === 视口裁剪 === */
        int sc0 = csx / TS;          if (sc0 < 0) sc0 = 0;
        int sr0 = csy / TS;          if (sr0 < 0) sr0 = 0;
        int sc1 = (csx + W + TS - 1) / TS;  if (sc1 > MAP_WIDTH)  sc1 = MAP_WIDTH;
        int sr1 = (csy + H + TS - 1) / TS;  if (sr1 > MAP_HEIGHT) sr1 = MAP_HEIGHT;

        /* === 渲染 === */
        gamelib_clear(g, COLOR_BLACK);

        if (tileset_id >= 0 && map_id >= 0) {
            int cols = MAP_TILESET_TILEMAP_COLUMNS;
            for (int r = sr0; r < sr1; r++) {
                for (int c = sc0; c < sc1; c++) {
                    int i = r * MAP_WIDTH + c;
                    int dx = c * TS - csx;
                    int dy = r * TS - csy;

                    int tid = map_layer_layer_0_base[i];
                    if (tid >= 0)
                        gamelib_draw_sprite_atlas_frame_scaled(g, tileset_id, dx, dy,
                            (tid % cols) * MAP_TILE_WIDTH,
                            (tid / cols) * MAP_TILE_HEIGHT,
                            MAP_TILE_WIDTH, MAP_TILE_HEIGHT, 0,
                            TS, TS, SPRITE_COLORKEY);

                    tid = map_layer_layer_1[i];
                    if (tid >= 0)
                        gamelib_draw_sprite_atlas_frame_scaled(g, tileset_id, dx, dy,
                            (tid % cols) * MAP_TILE_WIDTH,
                            (tid / cols) * MAP_TILE_HEIGHT,
                            MAP_TILE_WIDTH, MAP_TILE_HEIGHT, 0,
                            TS, TS, SPRITE_COLORKEY);
                }
            }
        }

        /* 绘制玩家 */
        {
            int row = atlas_row[player_dir][player_state];
            int dx = player_x * TILE_ZOOM - csx - DST_FRAME_W / 2;
            int dy = player_y * TILE_ZOOM - csy - DST_FRAME_H / 2;
            int flags = SPRITE_COLORKEY;
            if (player_flip) flags |= SPRITE_FLIP_H;
            gamelib_draw_sprite_atlas_frame_scaled(g, player_spr, dx, dy,
                0, row * FRAME_H,
                FRAME_W, FRAME_H, cur_frame,
                DST_FRAME_W, DST_FRAME_H, flags);
        }

        /* === HUD === */
        {
            const char *state_names[] = { "IDLE", "WALK", "ATTACK", "DEATH" };
            const char *dir_names[]   = { "DOWN", "RIGHT", "UP" };
            gamelib_draw_printf(g, 5, 5, COLOR_WHITE,
                                "Tile:%d,%d  Dir:%s%s",
                                player_x / MAP_TILE_WIDTH,
                                player_y / MAP_TILE_HEIGHT,
                                dir_names[player_dir],
                                player_flip ? "(L)" : "");
            gamelib_draw_printf(g, 5, 18, COLOR_GRAY,
                                "State:%s  Frame:%d/%d",
                                state_names[player_state],
                                cur_frame + 1, frame_counts[player_state]);
            gamelib_draw_printf(g, 5, H - 12, COLOR_DARKGRAY,
                                "DPAD:move  A:attack  B:die  SEL:quit");
        }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }

    if (map_id >= 0) gamelib_tilemap_free(g, map_id);
    gamelib_sprite_free(g, player_spr);
}
