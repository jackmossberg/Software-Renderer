#include "game.h"

void shader(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r,
            uint8_t g, uint8_t b) {
    (void)normal;
    (void)uv;
    (void)position;
    (void)light_dir;
    
    float dot;
    dot_vec3(&dot, normal, light_dir);
    float brightness = fmaxf(0.0f, fminf(1.0f, dot));

    brightness = fminf(1.0f, brightness + 0.21f);

    OUT[0] = r * brightness;
    OUT[1] = g * brightness;
    OUT[2] = b * brightness;

    OUT[3] = 255;
}

float angle = 0.0;
camera main_camera = {
    .near = 0.01f,
    .far = 150.0f,
    .fovy = 75.0f,
    .position = {0.0, 0.0, -3.0},
    .rotation = {0.0, 0.0, 0.0},
};

model test_model;

void init_game() {
  init_model(&test_model, NULL, (vec3){0.0, 0.0, 0.0}, (vec3){0.0, 0.0, 0.0},
             (vec3){1.0, 1.0, 1.0}, SHAPE_ICO_SPHERE);
}

void update_game(double deltatime, SDL_Event event) {
  (void)deltatime;
  (void)event;
  angle += (float)deltatime * 20.0;
}

void update_graphics(SDL_display *display) {
  (void)display;
  clear_display(display, 15, 20, 30);

  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);

  main_camera.position[0] =
      ((float)mouseX / (float)display->window_width) * 10.0f - 5.0f;
  main_camera.position[1] =
      ((float)mouseY / (float)display->window_height) * 10.0f - 5.0f;

  //test_model.rotation[0] = angle;
  test_model.rotation[1] = angle;
  //test_model.rotation[2] = angle;

  render_model(display, &test_model, &main_camera, false, shader);
}
