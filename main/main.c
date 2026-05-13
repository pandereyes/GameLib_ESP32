#include <stdio.h>
#include "gamelib.h"

/* ====== Select active demo (1-16) ====== */
#define ACTIVE_DEMO 16

/* ====== HAL registration ====== */
void gamelib_port_register_hal(void);

/* ====== Demo function prototypes ====== */
static void demo_01_hello(gamelib_t *g);
static void demo_02_movement(gamelib_t *g);
static void demo_03_shapes(gamelib_t *g);
static void demo_05_sprites(gamelib_t *g);
static void demo_07_shooting(gamelib_t *g);
static void demo_08_breakout(gamelib_t *g);
static void demo_09_snake(gamelib_t *g);
static void demo_10_tilemap(gamelib_t *g);
static void demo_12_sprite_xform(gamelib_t *g);
static void demo_13_clip(gamelib_t *g);
static void demo_15_ui(gamelib_t *g);
static void demo_16_fs_test(gamelib_t *g);

/* ====== Demo implementations ====== */
#include "demos/demo_01_hello.c"
#include "demos/demo_02_movement.c"
#include "demos/demo_03_shapes.c"
#include "demos/demo_05_sprites.c"
#include "demos/demo_07_shooting.c"
#include "demos/demo_08_breakout.c"
#include "demos/demo_09_snake.c"
#include "demos/demo_10_tilemap.c"
#include "demos/demo_12_sprite_xform.c"
#include "demos/demo_13_clip.c"
#include "demos/demo_15_ui.c"
#include "demos/demo_16_fs_test.c"

/* ====== App entry ====== */
static gamelib_t game;

void app_main(void)
{
    gamelib_port_register_hal();

    if (gamelib_init(&game, 240, 320, 60) != 0) {
        printf("GameLib init failed\n");
        return;
    }

#if ACTIVE_DEMO == 1
    demo_01_hello(&game);
#elif ACTIVE_DEMO == 2
    demo_02_movement(&game);
#elif ACTIVE_DEMO == 3
    demo_03_shapes(&game);
#elif ACTIVE_DEMO == 5
    demo_05_sprites(&game);
#elif ACTIVE_DEMO == 7
    demo_07_shooting(&game);
#elif ACTIVE_DEMO == 8
    demo_08_breakout(&game);
#elif ACTIVE_DEMO == 9
    demo_09_snake(&game);
#elif ACTIVE_DEMO == 10
    demo_10_tilemap(&game);
#elif ACTIVE_DEMO == 12
    demo_12_sprite_xform(&game);
#elif ACTIVE_DEMO == 13
    demo_13_clip(&game);
#elif ACTIVE_DEMO == 15
    demo_15_ui(&game);
#elif ACTIVE_DEMO == 16
    demo_16_fs_test(&game);
#else
    #error "Invalid ACTIVE_DEMO"
#endif

    gamelib_deinit(&game);
}
