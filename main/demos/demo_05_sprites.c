/* demo_05_sprites.c -- Sprites & Animation */
static int demo_05_make_ship(gamelib_t *g)
{
    int id = gamelib_sprite_create(g, 16, 16);
    if (id < 0) return -1;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_NONE);
    /* body */
    for (int y = 4; y < 14; y++)
        for (int x = 6; x < 10; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_CYAN);
    /* nose */
    for (int y = 2; y < 4; y++)
        for (int x = 7; x < 9; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_WHITE);
    /* wings */
    for (int x = 1; x < 6; x++) { gamelib_sprite_set_pixel(g, id, x, 9, COLOR_GRAY); gamelib_sprite_set_pixel(g, id, x, 10, COLOR_GRAY); }
    for (int x = 10; x < 15; x++) { gamelib_sprite_set_pixel(g, id, x, 9, COLOR_GRAY); gamelib_sprite_set_pixel(g, id, x, 10, COLOR_GRAY); }
    /* tail */
    gamelib_sprite_set_pixel(g, id, 5, 13, COLOR_DARKGRAY); gamelib_sprite_set_pixel(g, id, 10, 13, COLOR_DARKGRAY);
    /* flame */
    gamelib_sprite_set_pixel(g, id, 7, 14, COLOR_ORANGE); gamelib_sprite_set_pixel(g, id, 8, 14, COLOR_ORANGE);
    gamelib_sprite_set_pixel(g, id, 7, 15, COLOR_YELLOW); gamelib_sprite_set_pixel(g, id, 8, 15, COLOR_YELLOW);
    gamelib_sprite_set_color_key(g, id, COLOR_NONE);
    return id;
}

static int demo_05_make_anim(gamelib_t *g)
{
    int id = gamelib_sprite_create(g, 32, 8);
    if (id < 0) return -1;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 32; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_NONE);
    gamelib_color_t colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_WHITE};
    int sizes[] = {1, 2, 3, 2};
    for (int f = 0; f < 4; f++) {
        int cx = f * 8 + 4;
        for (int dy = -sizes[f]; dy <= sizes[f]; dy++)
            for (int dx = -sizes[f]; dx <= sizes[f]; dx++)
                if (dx*dx + dy*dy <= sizes[f]*sizes[f])
                    gamelib_sprite_set_pixel(g, id, cx + dx, 4 + dy, colors[f]);
    }
    gamelib_sprite_set_color_key(g, id, COLOR_NONE);
    return id;
}

static void demo_05_sprites(gamelib_t *g)
{
    int ship = demo_05_make_ship(g);
    int anim = demo_05_make_anim(g);
    int shipX = 100, shipY = 180;
    int animFrame = 0, animTimer = 0;
    bool modeB = false;

    while (!gamelib_is_closed(g)) {
        if (gamelib_is_key_pressed(g, KEY_B)) break;
        if (gamelib_is_key_pressed(g, KEY_A)) modeB = !modeB;

        if (!modeB) {
            if (gamelib_is_key_down(g, KEY_LEFT))  shipX -= 2;
            if (gamelib_is_key_down(g, KEY_RIGHT)) shipX += 2;
            if (gamelib_is_key_down(g, KEY_UP))    shipY -= 2;
            if (gamelib_is_key_down(g, KEY_DOWN))  shipY += 2;
        } else {
            if (gamelib_is_key_down(g, KEY_LEFT))  shipX -= 3;
            if (gamelib_is_key_down(g, KEY_RIGHT)) shipX += 3;
            if (gamelib_is_key_down(g, KEY_UP))    shipY -= 3;
            if (gamelib_is_key_down(g, KEY_DOWN))  shipY += 3;
        }

        if (shipX < 0) shipX = 0;
        if (shipX > 240 - 16) shipX = 240 - 16;
        if (shipY < 0) shipY = 0;
        if (shipY > 320 - 16) shipY = 320 - 16;

        if (!modeB) {
            animTimer++; if (animTimer >= 10) { animTimer = 0; animFrame = (animFrame + 1) % 4; }

            gamelib_clear(g, COLOR_BLACK);

            gamelib_draw_text(g, 2, 2, "DrawSprite:", COLOR_WHITE);
            gamelib_draw_sprite(g, ship, 2, 16);
            gamelib_draw_text(g, 2, 40, "Flip H/V/H+V:", COLOR_WHITE);
            gamelib_draw_sprite_ex(g, ship, 2, 52, SPRITE_FLIP_H);
            gamelib_draw_sprite_ex(g, ship, 30, 52, SPRITE_FLIP_V);
            gamelib_draw_sprite_ex(g, ship, 58, 52, SPRITE_FLIP_H | SPRITE_FLIP_V);
            gamelib_draw_text(g, 2, 76, "Scaled:", COLOR_WHITE);
            gamelib_draw_sprite_scaled(g, ship, 2, 90, 32, 32);
            gamelib_draw_sprite_scaled(g, ship, 38, 90, 48, 48);
            gamelib_draw_text(g, 2, 146, "Frame:", COLOR_WHITE);
            gamelib_draw_sprite_frame_scaled(g, anim, 2, 158, 8, 8, animFrame, 32, 32, 0);
            gamelib_draw_sprite_frame_scaled(g, anim, 38, 158, 8, 8, animFrame, 32, 32, SPRITE_FLIP_H);
            gamelib_draw_sprite(g, ship, shipX, shipY);
            gamelib_draw_text(g, 2, 300, "START:switch  Arrows:move", COLOR_LIGHTGRAY);
        } else {
            gamelib_clear(g, COLOR_DARK_GREEN);
            /* draw a grid background */
            for (int i = 0; i < 240; i += 12)

                gamelib_draw_line(g, i, 0, i, 320, COLOR_GREEN);
            for (int i = 0; i < 320; i += 12)
                gamelib_draw_line(g, 0, i, 240, i, COLOR_GREEN);
            gamelib_draw_sprite(g, ship, shipX, shipY);
            gamelib_draw_text(g, 2, 2, "Mode B: Move ship", COLOR_WHITE);
        }

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
    gamelib_sprite_free(g, ship);
    gamelib_sprite_free(g, anim);
}
