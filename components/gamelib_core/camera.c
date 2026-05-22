#include "camera.h"

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static void camera_clamp(gamelib_camera_t *camera)
{
    if (!camera) return;
    int max_x = camera->world_w - camera->view_w;
    int max_y = camera->world_h - camera->view_h;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    camera->x = clamp_int(camera->x, 0, max_x);
    camera->y = clamp_int(camera->y, 0, max_y);
}

void gamelib_camera_init(gamelib_camera_t *camera, int view_w, int view_h,
                         int world_w, int world_h)
{
    if (!camera) return;
    camera->x = 0;
    camera->y = 0;
    camera->view_w = view_w;
    camera->view_h = view_h;
    camera->world_w = world_w;
    camera->world_h = world_h;
    camera_clamp(camera);
}

void gamelib_camera_set_view(gamelib_camera_t *camera, int view_w, int view_h)
{
    if (!camera) return;
    camera->view_w = view_w;
    camera->view_h = view_h;
    camera_clamp(camera);
}

void gamelib_camera_set_world(gamelib_camera_t *camera, int world_w, int world_h)
{
    if (!camera) return;
    camera->world_w = world_w;
    camera->world_h = world_h;
    camera_clamp(camera);
}

void gamelib_camera_set_position(gamelib_camera_t *camera, int x, int y)
{
    if (!camera) return;
    camera->x = x;
    camera->y = y;
    camera_clamp(camera);
}

void gamelib_camera_follow(gamelib_camera_t *camera, int target_x, int target_y)
{
    if (!camera) return;
    camera->x = target_x - camera->view_w / 2;
    camera->y = target_y - camera->view_h / 2;
    camera_clamp(camera);
}

int gamelib_camera_world_to_screen_x(const gamelib_camera_t *camera, int world_x)
{
    return camera ? world_x - camera->x : world_x;
}

int gamelib_camera_world_to_screen_y(const gamelib_camera_t *camera, int world_y)
{
    return camera ? world_y - camera->y : world_y;
}
