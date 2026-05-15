#include "gamelib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 240
#define H 320

#define MAX_STARS      80
#define MAX_BULLETS    30
#define MAX_ENEMIES    20
#define MAX_EXPLOSIONS 15
#define MAX_ENEMY_BULLETS 20

typedef struct { float x, y, speed; gamelib_color_t color; } Star;
typedef struct { float x, y; bool active; } Bullet;
typedef struct { float x, y, vx, vy; int hp; bool active; int type; } Enemy;
typedef struct { float x, y; int timer; bool active; } Explosion;
typedef struct { float x, y, vy; bool active; } EnemyBullet;

#include "esp_heap_caps.h"

static uint8_t* load_file_data(gamelib_t *g, const char *path, size_t *out_len) {
    void *f = gamelib_file_open(g, path, "rb");
    if (!f) return NULL;
    int size = gamelib_file_size(g, f);
    if (size <= 0) {
        gamelib_file_close(g, f);
        return NULL;
    }
    uint8_t *buf = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = (uint8_t*)malloc(size); // fallback
    if (!buf) {
        gamelib_file_close(g, f);
        return NULL;
    }
    int read = gamelib_file_read(g, f, buf, size);
    gamelib_file_close(g, f);
    if (read != size) {
        free(buf);
        return NULL;
    }
    if (out_len) *out_len = size;
    return buf;
}

static int load_sprite_fallback(gamelib_t *g, const char *path, const char *ignored) {
    size_t len;
    uint8_t *buf = load_file_data(g, path, &len);
    if (!buf) return -1;
    
    int id;
    if (strstr(path, ".png")) {
        id = gamelib_sprite_load_png(g, buf, len);
    } else {
        id = gamelib_sprite_load_bmp(g, buf, len);
    }
    free(buf);
    return id;
}

static void queue_sound(const uint8_t **queued_sound, int *queued_priority, const uint8_t *candidate, int candidate_priority) {
    if (!candidate) return;
    if (candidate_priority >= *queued_priority) {
        *queued_sound = candidate;
        *queued_priority = candidate_priority;
    }
}

static void spawn_explosion(Explosion explosions[], float x, float y, int timer) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) {
            explosions[i].active = true;
            explosions[i].x = x; explosions[i].y = y;
            explosions[i].timer = timer;
            break;
        }
    }
}

static int create_player_sprite(gamelib_t *g) {
    int id = gamelib_sprite_create(g, 24, 24);
    if (id < 0) return -1;
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 24; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_NONE);
    for (int y = 4; y < 20; y++)
        for (int x = 9; x < 15; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_CYAN);
    for (int x = 10; x < 14; x++) {
        gamelib_sprite_set_pixel(g, id, x, 2, COLOR_WHITE);
        gamelib_sprite_set_pixel(g, id, x, 3, COLOR_WHITE);
    }
    gamelib_sprite_set_pixel(g, id, 11, 0, COLOR_WHITE);
    gamelib_sprite_set_pixel(g, id, 12, 0, COLOR_WHITE);
    gamelib_sprite_set_pixel(g, id, 11, 1, COLOR_WHITE);
    gamelib_sprite_set_pixel(g, id, 12, 1, COLOR_WHITE);
    for (int x = 2; x < 9; x++)
        for (int y = 13; y < 17; y++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_GRAY);
    for (int x = 15; x < 22; x++)
        for (int y = 13; y < 17; y++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_GRAY);
    gamelib_sprite_set_pixel(g, id, 11, 20, COLOR_ORANGE);
    gamelib_sprite_set_pixel(g, id, 12, 20, COLOR_ORANGE);
    gamelib_sprite_set_pixel(g, id, 11, 21, COLOR_YELLOW);
    gamelib_sprite_set_pixel(g, id, 12, 21, COLOR_YELLOW);
    gamelib_sprite_set_color_key(g, id, COLOR_NONE);
    return id;
}

static int create_enemy_sprite(gamelib_t *g, gamelib_color_t body_color) {
    int id = gamelib_sprite_create(g, 20, 20);
    if (id < 0) return -1;
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 20; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_NONE);
    for (int y = 2; y < 14; y++) {
        int half = (14 - y), cx = 10;
        for (int x = cx - half; x < cx + half; x++)
            if (x >= 0 && x < 20)
                gamelib_sprite_set_pixel(g, id, x, y, body_color);
    }
    for (int y = 3; y < 8; y++) {
        gamelib_sprite_set_pixel(g, id, 1, y, COLOR_DARKGRAY);
        gamelib_sprite_set_pixel(g, id, 18, y, COLOR_DARKGRAY);
    }
    gamelib_sprite_set_pixel(g, id, 9,  5, COLOR_YELLOW);
    gamelib_sprite_set_pixel(g, id, 10, 5, COLOR_YELLOW);
    gamelib_sprite_set_pixel(g, id, 9,  6, COLOR_YELLOW);
    gamelib_sprite_set_pixel(g, id, 10, 6, COLOR_YELLOW);
    gamelib_sprite_set_color_key(g, id, COLOR_NONE);
    return id;
}

static void demo_14_space_shooter(gamelib_t *g) {
    uint8_t *shoot_sfx     = load_file_data(g, "assert/sound/click.wav", NULL);
    uint8_t *enemy_hit_sfx = load_file_data(g, "assert/sound/hit.wav", NULL);
    uint8_t *enemy_down_sfx= load_file_data(g, "assert/sound/coin.wav", NULL);
    uint8_t *level_up_sfx  = load_file_data(g, "assert/sound/note_do_high.wav", NULL);
    uint8_t *player_hit_sfx= load_file_data(g, "assert/sound/explosion.wav", NULL);
    uint8_t *game_over_sfx = load_file_data(g, "assert/sound/game_over.wav", NULL);
    uint8_t *restart_sfx   = load_file_data(g, "assert/sound/click.wav", NULL);

    int sprPlayer = load_sprite_fallback(g, "assert/plane0.png", NULL);
    if (sprPlayer < 0) {
        sprPlayer = create_player_sprite(g);
    }

    int sprBullet = load_sprite_fallback(g, "assert/bullet.png", NULL);
    int sprExplosion = load_sprite_fallback(g, "assert/explosion.png", NULL);

    int sprEnemy1 = create_enemy_sprite(g, COLOR_RED);
    int sprEnemy2 = create_enemy_sprite(g, COLOR_MAGENTA);

    int playerW = (sprPlayer >= 0) ? gamelib_sprite_width(g, sprPlayer) : 24;
    int playerH = (sprPlayer >= 0) ? gamelib_sprite_height(g, sprPlayer) : 24;
    int enemyW = 20, enemyH = 20;
    int playerBulletW = 4, playerBulletH = 10;
    int enemyBulletW = 4, enemyBulletH = 10;
    int explosionLife = 16;

    static Star stars[MAX_STARS];
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = (float)gamelib_random(0, W - 1);
        stars[i].y = (float)gamelib_random(0, H - 1);
        stars[i].speed = (float)gamelib_random(1, 4);
        int b = 80 + (int)(stars[i].speed * 40);
        if (b > 255) b = 255;
        stars[i].color = COLOR_RGB(b, b, b);
    }

    float px = W / 2.0f - playerW / 2.0f, py = H - playerH - 10.0f;

    static Bullet bullets[MAX_BULLETS];
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    int shootTimer = 0, shootSfxCooldown = 0;

    static Enemy enemies[MAX_ENEMIES];
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    int spawnTimer = 0;

    static EnemyBullet eBullets[MAX_ENEMY_BULLETS];
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) eBullets[i].active = false;

    static Explosion explosions[MAX_EXPLOSIONS];
    for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;

    int score = 0, lives = 3, level = 1, killCount = 0;
    bool gameOver = false;
    int invincible = 0;

    while (!gamelib_is_closed(g)) {
        const uint8_t *sfxToPlay = NULL;
        int sfxPriority = 0;
        
        if (gamelib_is_key_pressed(g, KEY_SELECT)) break;

        if (!gameOver) {
            if (shootSfxCooldown > 0) shootSfxCooldown--;

            float spd = 3.0f;
            if (gamelib_is_key_down(g, KEY_LEFT))  px -= spd;
            if (gamelib_is_key_down(g, KEY_RIGHT)) px += spd;
            if (gamelib_is_key_down(g, KEY_UP))    py -= spd;
            if (gamelib_is_key_down(g, KEY_DOWN))  py += spd;
            if (px < 0) px = 0;
            if (px > W - playerW) px = (float)(W - playerW);
            if (py < 0) py = 0;
            if (py > H - playerH) py = (float)(H - playerH);

            if (gamelib_is_key_down(g, KEY_A)) {
                shootTimer++;
                if (shootTimer >= 6) {
                    shootTimer = 0;
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].active = true;
                            bullets[i].x = px + playerW / 2.0f - playerBulletW / 2.0f;
                            bullets[i].y = py - playerBulletH + 2;
                            if (shootSfxCooldown <= 0) {
                                queue_sound(&sfxToPlay, &sfxPriority, shoot_sfx, 1);
                                shootSfxCooldown = 10;
                            }
                            break;
                        }
                    }
                }
            } else { shootTimer = 5; }

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) continue;
                bullets[i].y -= 8;
                if (bullets[i].y < -playerBulletH) bullets[i].active = false;
            }

            spawnTimer++;
            int rate = 50 - level * 5;
            if (rate < 15) rate = 15;
            if (spawnTimer >= rate) {
                spawnTimer = 0;
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!enemies[i].active) {
                        enemies[i].active = true;
                        enemies[i].x = (float)gamelib_random(10, W - 30);
                        enemies[i].y = (float)gamelib_random(-80, -20);
                        enemies[i].vx = (float)(gamelib_random(-2, 2));
                        enemies[i].vy = (float)(gamelib_random(1, 2 + level / 2));
                        enemies[i].type = gamelib_random(0, 1);
                        enemies[i].hp = enemies[i].type == 0 ? 1 : 2;
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) continue;
                enemies[i].x += enemies[i].vx;
                enemies[i].y += enemies[i].vy;
                if (enemies[i].x < 0 || enemies[i].x > W - enemyW) enemies[i].vx = -enemies[i].vx;
                if (enemies[i].y > H + enemyH) enemies[i].active = false;
                if (gamelib_random(0, 200) < 1 + level) {
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!eBullets[j].active) {
                            eBullets[j].active = true;
                            eBullets[j].x = enemies[i].x + enemyW / 2.0f - enemyBulletW / 2.0f;
                            eBullets[j].y = enemies[i].y + enemyH - 4;
                            eBullets[j].vy = 4.0f + level * 0.5f;
                            break;
                        }
                    }
                }
            }

            for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                if (!eBullets[i].active) continue;
                eBullets[i].y += eBullets[i].vy;
                if (eBullets[i].y > H + enemyBulletH) eBullets[i].active = false;
            }

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) continue;
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (!enemies[j].active) continue;
                    if (gamelib_rect_overlap(
                            (int)bullets[i].x, (int)bullets[i].y, playerBulletW, playerBulletH,
                            (int)enemies[j].x, (int)enemies[j].y, enemyW, enemyH)) {
                        bullets[i].active = false;
                        enemies[j].hp--;
                        if (enemies[j].hp <= 0) {
                            enemies[j].active = false;
                            score += (enemies[j].type + 1) * 100;
                            killCount++;
                            queue_sound(&sfxToPlay, &sfxPriority, enemy_down_sfx, 3);
                            if (killCount >= 10 + level * 5) {
                                level++; killCount = 0;
                                queue_sound(&sfxToPlay, &sfxPriority, level_up_sfx, 4);
                            }
                            spawn_explosion(explosions,
                                           enemies[j].x + enemyW / 2.0f,
                                           enemies[j].y + enemyH / 2.0f, explosionLife);
                        } else {
                            queue_sound(&sfxToPlay, &sfxPriority, enemy_hit_sfx, 2);
                        }
                        break;
                    }
                }
            }

            if (invincible > 0) { invincible--; }
            else {
                for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                    if (!eBullets[i].active) continue;
                    if (gamelib_rect_overlap(
                            (int)eBullets[i].x, (int)eBullets[i].y, enemyBulletW, enemyBulletH,
                            (int)px + 4, (int)py + 4, playerW - 8, playerH - 8)) {
                        eBullets[i].active = false;
                        lives--; invincible = 60;
                        spawn_explosion(explosions, px + playerW / 2.0f, py + playerH / 2.0f, explosionLife);
                        if (lives <= 0) { gameOver = true; queue_sound(&sfxToPlay, &sfxPriority, game_over_sfx, 6); }
                        else { queue_sound(&sfxToPlay, &sfxPriority, player_hit_sfx, 5); }
                        break;
                    }
                }
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!enemies[i].active) continue;
                    if (gamelib_rect_overlap(
                            (int)enemies[i].x, (int)enemies[i].y, enemyW, enemyH,
                            (int)px + 4, (int)py + 4, playerW - 8, playerH - 8)) {
                        enemies[i].active = false;
                        lives--; invincible = 60;
                        spawn_explosion(explosions, enemies[i].x + enemyW / 2.0f, enemies[i].y + enemyH / 2.0f, explosionLife);
                        spawn_explosion(explosions, px + playerW / 2.0f, py + playerH / 2.0f, explosionLife);
                        if (lives <= 0) { gameOver = true; queue_sound(&sfxToPlay, &sfxPriority, game_over_sfx, 6); }
                        else { queue_sound(&sfxToPlay, &sfxPriority, player_hit_sfx, 5); }
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_EXPLOSIONS; i++) {
                if (!explosions[i].active) continue;
                explosions[i].timer--;
                if (explosions[i].timer <= 0) explosions[i].active = false;
            }
        } else {
            if (gamelib_is_key_pressed(g, KEY_B)) {
                px = W / 2.0f - playerW / 2.0f; py = H - playerH - 10.0f;
                for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
                for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
                for (int i = 0; i < MAX_ENEMY_BULLETS; i++) eBullets[i].active = false;
                for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;
                score = 0; lives = 3; level = 1; killCount = 0;
                spawnTimer = 0; shootTimer = 0; shootSfxCooldown = 0;
                gameOver = false; invincible = 0;
                queue_sound(&sfxToPlay, &sfxPriority, restart_sfx, 1);
            }
        }

        if (sfxToPlay) gamelib_play_wav(g, sfxToPlay, 0, 255);

        for (int i = 0; i < MAX_STARS; i++) {
            stars[i].y += stars[i].speed;
            if (stars[i].y > H) { stars[i].y = 0; stars[i].x = (float)gamelib_random(0, W - 1); }
        }

        gamelib_clear(g, COLOR_BLACK);

        for (int i = 0; i < MAX_STARS; i++)
            gamelib_set_pixel(g, (int)stars[i].x, (int)stars[i].y, stars[i].color);

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;
            if (sprBullet >= 0)
                gamelib_draw_sprite_scaled(g, sprBullet, (int)bullets[i].x, (int)bullets[i].y,
                                      playerBulletW, playerBulletH);
            else
                gamelib_fill_rect(g, (int)bullets[i].x, (int)bullets[i].y, playerBulletW, playerBulletH, COLOR_YELLOW);
        }

        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!eBullets[i].active) continue;
            if (sprBullet >= 0)
                gamelib_draw_sprite_scaled(g, sprBullet, (int)eBullets[i].x, (int)eBullets[i].y,
                                      enemyBulletW, enemyBulletH);
            else
                gamelib_fill_rect(g, (int)eBullets[i].x, (int)eBullets[i].y, enemyBulletW, enemyBulletH, COLOR_RED);
        }

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            int spr = enemies[i].type == 0 ? sprEnemy1 : sprEnemy2;
            gamelib_draw_sprite_ex(g, spr, (int)enemies[i].x, (int)enemies[i].y, SPRITE_COLORKEY | SPRITE_ALPHA);
        }

        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (!explosions[i].active) continue;
            if (sprExplosion >= 0) {
                int frame = (explosionLife - explosions[i].timer) / 4;
                if (frame < 0) frame = 0;
                if (frame > 3) frame = 3;
                gamelib_draw_sprite_frame_scaled(g, sprExplosion, (int)explosions[i].x - 12, (int)explosions[i].y - 12,
                                           32, 32, frame, 24, 24, SPRITE_COLORKEY | SPRITE_ALPHA);
            } else {
                int r = explosionLife - explosions[i].timer + 5;
                gamelib_color_t c;
                if (explosions[i].timer > 10) c = COLOR_WHITE;
                else if (explosions[i].timer > 5) c = COLOR_YELLOW;
                else c = COLOR_ORANGE;
                gamelib_fill_circle(g, (int)explosions[i].x, (int)explosions[i].y, r, c);
                if (r > 3) gamelib_fill_circle(g, (int)explosions[i].x, (int)explosions[i].y, r - 3, COLOR_RED);
            }
        }

        if (invincible == 0 || (invincible / 4) % 2 == 0)
            gamelib_draw_sprite_ex(g, sprPlayer, (int)px, (int)py, SPRITE_COLORKEY | SPRITE_ALPHA);

        gamelib_draw_printf(g, 5, 5, COLOR_WHITE, "SCORE: %d", score);
        gamelib_draw_printf(g, W - 65, 5, COLOR_GREEN, "LIVES: %d", lives);
        gamelib_draw_printf(g, W / 2 - 20, 5, COLOR_YELLOW, "LV.%d", level);

        if (gameOver) {
            gamelib_fill_rect(g, W / 2 - 70, H / 2 - 30, 140, 60, COLOR_DARKGRAY);
            gamelib_draw_rect(g, W / 2 - 70, H / 2 - 30, 140, 60, COLOR_WHITE);
            gamelib_draw_text(g, W / 2 - 35, H / 2 - 20, "GAME OVER", COLOR_RED);
            gamelib_draw_printf(g, W / 2 - 45, H / 2 - 5, COLOR_WHITE, "Score: %d", score);
            gamelib_draw_text(g, W / 2 - 45, H / 2 + 10, "KEY B restart", COLOR_YELLOW);
        }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }

    if (sprPlayer >= 0) gamelib_sprite_free(g, sprPlayer);
    if (sprBullet >= 0) gamelib_sprite_free(g, sprBullet);
    if (sprExplosion >= 0) gamelib_sprite_free(g, sprExplosion);
    if (sprEnemy1 >= 0) gamelib_sprite_free(g, sprEnemy1);
    if (sprEnemy2 >= 0) gamelib_sprite_free(g, sprEnemy2);
    if (shoot_sfx) free(shoot_sfx);
    if (enemy_hit_sfx) free(enemy_hit_sfx);
    if (enemy_down_sfx) free(enemy_down_sfx);
    if (level_up_sfx) free(level_up_sfx);
    if (player_hit_sfx) free(player_hit_sfx);
    if (game_over_sfx) free(game_over_sfx);
    if (restart_sfx) free(restart_sfx);
}