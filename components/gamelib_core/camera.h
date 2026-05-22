#ifndef GAMELIB_CAMERA_H
#define GAMELIB_CAMERA_H

typedef struct {
    int x;
    int y;
    int view_w;
    int view_h;
    int world_w;
    int world_h;
} gamelib_camera_t;

void gamelib_camera_init(gamelib_camera_t *camera, int view_w, int view_h,
                         int world_w, int world_h);
void gamelib_camera_set_view(gamelib_camera_t *camera, int view_w, int view_h);
void gamelib_camera_set_world(gamelib_camera_t *camera, int world_w, int world_h);
void gamelib_camera_set_position(gamelib_camera_t *camera, int x, int y);
void gamelib_camera_follow(gamelib_camera_t *camera, int target_x, int target_y);
int  gamelib_camera_world_to_screen_x(const gamelib_camera_t *camera, int world_x);
int  gamelib_camera_world_to_screen_y(const gamelib_camera_t *camera, int world_y);

#endif
