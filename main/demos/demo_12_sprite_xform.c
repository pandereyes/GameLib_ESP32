/* demo_12_sprite_xform.c -- Sprite Rotation */

static int demo_12_make_arrow(gamelib_t *g)
{
    int id = gamelib_sprite_create(g, 16, 16);
    if (id < 0) return -1;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            gamelib_sprite_set_pixel(g, id, x, y, COLOR_NONE);
    for (int y = 0; y < 16; y++) {
        int hw = y < 8 ? y + 1 : 16 - y;
        for (int x = 0; x < hw * 2; x++)
            gamelib_sprite_set_pixel(g, id, x + 2, y, COLOR_YELLOW);
    }
    gamelib_sprite_set_color_key(g, id, COLOR_NONE);
    return id;
}

static int demo_12_make_pulse(gamelib_t *g)
{
    int id = gamelib_sprite_create(g, 64, 16);
    if (id < 0) return -1;
    gamelib_color_t colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_WHITE};
    int sizes[] = {1, 2, 3, 2};
    for (int f = 0; f < 4; f++) {
        int cx = f * 16 + 8;
        for (int dy = -sizes[f]; dy <= sizes[f]; dy++)
            for (int dx = -sizes[f]; dx <= sizes[f]; dx++)
                if (dx*dx + dy*dy <= sizes[f]*sizes[f])
                    gamelib_sprite_set_pixel(g, id, cx + dx, 8 + dy, colors[f]);
    }
    gamelib_sprite_set_color_key(g, id, COLOR_NONE);
    return id;
}

static void demo_12_sprite_xform(gamelib_t *g)
{
    int arrow = demo_12_make_arrow(g);
    int pulse = demo_12_make_pulse(g);
    double angle = 0.0;
    int frame = 0, ftimer = 0;

    while (!gamelib_is_closed(g)) {
        gamelib_clear(g, COLOR_RGB(28, 34, 50));

        gamelib_draw_text(g, 2, 2, "12 Sprite Rotation", COLOR_WHITE);

        /* Arrow rotations */
        gamelib_draw_text(g, 2, 16, "Arrow:", COLOR_LIGHTGRAY);
        gamelib_draw_sprite_rotated(g, arrow, 40, 60, angle, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(g, arrow, 80, 60, angle + 45.0, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(g, arrow, 120, 60, angle + 90.0, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(g, arrow, 160, 60, angle + 135.0, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(g, arrow, 200, 60, angle + 270.0, SPRITE_COLORKEY);

        /* Frame rotated */
        ftimer++; if (ftimer >= 8) { ftimer = 0; frame = (frame + 1) % 4; }
        gamelib_draw_text(g, 2, 90, "Frame rot:", COLOR_LIGHTGRAY);
        gamelib_draw_sprite_frame_rotated(g, pulse, 50, 160, 16, 16, frame, angle, 0);
        gamelib_draw_sprite_frame_rotated(g, pulse, 100, 160, 16, 16, frame, angle + 45.0, 0);
        gamelib_draw_sprite_frame_rotated(g, pulse, 150, 160, 16, 16, frame, angle + 90.0, 0);

        /* HUD */
        gamelib_draw_printf(g, 2, 310, COLOR_LIGHTGRAY, "Angle:%.0f <- ->  B:quit", angle);
        if (gamelib_is_key_down(g, KEY_LEFT))  angle -= 2.0;
        if (gamelib_is_key_down(g, KEY_RIGHT)) angle += 2.0;
        if (angle >= 360) angle -= 360;
        if (angle < 0) angle += 360;

        if (gamelib_is_key_pressed(g, KEY_B)) break;
        gamelib_update(g);
        gamelib_wait_frame(g);
    }
    gamelib_sprite_free(g, arrow);
    gamelib_sprite_free(g, pulse);
}
