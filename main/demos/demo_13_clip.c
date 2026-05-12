/* demo_13_clip.c -- Clipping Rectangles */

static void demo_13_draw_grid_bg(gamelib_t *g)
{
    for (int y = 0; y < 320; y += 20)
        for (int x = 0; x < 240; x += 20)
            gamelib_fill_rect(g, x, y, 20, 20, ((x/20 + y/20) & 1) ? COLOR_RGB(40, 44, 52) : COLOR_RGB(50, 54, 62));
}

static void demo_13_clip(gamelib_t *g)
{
    int clipW = 100, clipH = 100;
    int clipX = 70, clipY = 60;
    bool clipOn = true;
    double angle = 0.0;

    /* make a cross sprite */
    int cross = gamelib_sprite_create(g, 20, 20);
    if (cross >= 0) {
        for (int y = 0; y < 20; y++)
            for (int x = 0; x < 20; x++)
                gamelib_sprite_set_pixel(g, cross, x, y, COLOR_NONE);
        for (int i = 0; i < 20; i++) {
            gamelib_sprite_set_pixel(g, cross, i, 9, COLOR_YELLOW);
            gamelib_sprite_set_pixel(g, cross, i, 10, COLOR_YELLOW);
            gamelib_sprite_set_pixel(g, cross, 9, i, COLOR_YELLOW);
            gamelib_sprite_set_pixel(g, cross, 10, i, COLOR_YELLOW);
        }
        gamelib_sprite_set_color_key(g, cross, COLOR_NONE);
    }

    while (!gamelib_is_closed(g)) {
        demo_13_draw_grid_bg(g);

        if (clipOn) gamelib_set_clip(g, clipX, clipY, clipW, clipH);

        /* draw shapes that get clipped */
        for (int i = 0; i < 240; i += 25)
            gamelib_draw_line(g, i, 0, i, 320, COLOR_RGB(255, 80, 80));
        for (int i = 0; i < 320; i += 25)
            gamelib_draw_line(g, 0, i, 240, i, COLOR_RGB(80, 80, 255));

        gamelib_fill_circle(g, 120, 110, 60, COLOR_RGB(100, 180, 100));
        gamelib_draw_sprite_rotated(g, cross, 120, 110, angle, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(g, cross, 80, 80, angle + 45.0, SPRITE_COLORKEY);
        gamelib_draw_sprite_rotated(g, cross, 160, 140, -angle, SPRITE_COLORKEY);

        if (clipOn) gamelib_clear_clip(g);

        /* clip border */
        gamelib_draw_rect(g, clipX, clipY, clipW, clipH, clipOn ? COLOR_GOLD : COLOR_GRAY);
        gamelib_draw_printf(g, 2, 2, COLOR_WHITE, "Clip %s  A:toggle  Arrows:move/resize  <- ->:rotate",
                           clipOn ? "ON" : "OFF");
        gamelib_draw_printf(g, 2, 310, COLOR_LIGHTGRAY, "clip:%d,%d %dx%d  angle:%.0f",
                           gamelib_get_clip_x(g), gamelib_get_clip_y(g),
                           gamelib_get_clip_w(g), gamelib_get_clip_h(g), angle);

        if (gamelib_is_key_pressed(g, KEY_A)) clipOn = !clipOn;
        if (gamelib_is_key_down(g, KEY_LEFT))  { if (gamelib_is_key_down(g, KEY_B)) clipW -= 2; else angle -= 2.0; }
        if (gamelib_is_key_down(g, KEY_RIGHT)) { if (gamelib_is_key_down(g, KEY_B)) clipW += 2; else angle += 2.0; }
        if (gamelib_is_key_down(g, KEY_UP))    { if (gamelib_is_key_down(g, KEY_B)) clipH -= 2; else clipY -= 1; }
        if (gamelib_is_key_down(g, KEY_DOWN))  { if (gamelib_is_key_down(g, KEY_B)) clipH += 2; else clipY += 1; }
        if (angle >= 360) angle -= 360;
        if (angle < 0) angle += 360;
        if (clipW < 20) clipW = 20;
        if (clipH < 20) clipH = 20;
        if (gamelib_is_key_pressed(g, KEY_START)) break;

        gamelib_update(g);
        gamelib_wait_frame(g);
    }
    gamelib_sprite_free(g, cross);
}
