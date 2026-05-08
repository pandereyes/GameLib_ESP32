idf_component_register(
    SRCS "gamelib.c" "framebuffer.c" "draw.c" "text.c" "sprite.c" "bmp.c" "audio.c"
    INCLUDE_DIRS "."
    REQUIRES gamelib_hal
)
