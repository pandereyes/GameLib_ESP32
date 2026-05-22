idf_component_register(
    SRCS "asset.c" "anim.c" "camera.c" "collision.c" "ui.c" "font.c" "tilemap.c" "wav.c" "gamelib.c" "framebuffer.c" "draw.c" "text.c" "sprite.c" "bmp.c" "png.c" "lodepng.c" "audio.c"
    INCLUDE_DIRS "."
    REQUIRES gamelib_hal
)
