#include "game.h"

float angle = 0.0;

camera main_camera = {
    .near = 0.01f,
    .far = 150.0f,
    .fovy = 60.0f,
    .position = {0.0, 0.0, -3.0},
    .rotation = {0.0, 0.0, 0.0},
};

void init_game() {}

void update_game(double deltatime, SDL_Event event) {
  (void)deltatime;
  (void)event;
  angle += (float)deltatime * 20.0;
}

void update_graphics(SDL_display *display) {
  (void)display;
  clear_display(display, 0, 0, 0);

  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);

  set_tri3d(display, main_camera, 255, 100, 0, (vec3){0.0f, 0.5f, 0.0f},
            (vec3){-0.5f, -0.5f, 0.0f}, (vec3){0.5f, -0.5f, 0.0f},
            (vec3){0.0, 0.0, 1.0f}, (vec3){-angle, angle, -angle}, false);  
}
