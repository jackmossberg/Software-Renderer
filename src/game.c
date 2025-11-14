#include "game.h"

#define PITCH_MIN -89.0f
#define PITCH_MAX  89.0f
#define MOUSE_SENS 0.002f
#define MOVE_SPEED 2.1f

typedef struct player {
  camera* cam;
  vec3 position;
  vec3 rotation;
  int mouse_captured;
} player;

player main_player;

void update_player_controller(player *p, double deltatime, SDL_Event event) {
  float speed = MOVE_SPEED * deltatime;

  const Uint8 *state = SDL_GetKeyboardState(NULL);
  vec3 forward = {.0f, 0.0f, 1.0f}, right = {1.0f, 0.0f, 0.0f};
  
  if (state[SDL_SCANCODE_W]) {
    p->position[0] += forward[0] * speed;
    p->position[1] += forward[1] * speed;
    p->position[2] += forward[2] * speed;
  }
  if (state[SDL_SCANCODE_S]) {
    p->position[0] -= forward[0] * speed;
    p->position[1] -= forward[1] * speed;
    p->position[2] -= forward[2] * speed;
  }
  if (state[SDL_SCANCODE_A]) {
    p->position[0] -= right[0] * speed;
    p->position[1] -= right[1] * speed;
    p->position[2] -= right[2] * speed;
  }
  if (state[SDL_SCANCODE_D]) {
    p->position[0] += right[0] * speed;
    p->position[1] += right[1] * speed;
    p->position[2] += right[2] * speed;
  }
  if (state[SDL_SCANCODE_SPACE]) p->position[1] -= speed;
  if (state[SDL_SCANCODE_LCTRL]) p->position[1] += speed;

  p->cam->position[0] = p->position[0];
  p->cam->position[1] = p->position[1] - 1.2f;
  p->cam->position[2] = p->position[2];
}

void terrain_geo_shader(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, 
            uint8_t r, uint8_t g, uint8_t b) {
    (void)uv;
    (void)position;

    float dot;
    dot_vec3(&dot, normal, light_dir);
    float brightness = fmaxf(0.0f, fminf(1.0f, dot));
    brightness = fminf(1.0f, brightness + 0.21f);

    OUT[0] = r * brightness;
    OUT[1] = g * brightness;
    OUT[2] = b * brightness;
    OUT[3] = 255.0f;
}

void terrain_frag_shader(vec4 OUT, vec4 IN, vec2 uv, vec3 position,
                          vec3 normal) {
    OUT[0] = position[0] * IN[0];
    OUT[1] = position[1] * IN[1];
    OUT[2] = position[2] * IN[2];

    OUT[3] = IN[3];
}

void model_geo_shader(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, 
            uint8_t r, uint8_t g, uint8_t b) {
    (void)uv;
    (void)position;
    
    float dot;
    dot_vec3(&dot, normal, light_dir);
    float brightness = fmaxf(0.0f, fminf(1.0f, dot));
    brightness = fminf(1.0f, brightness + 0.21f);

    OUT[0] = r * brightness;
    OUT[1] = g * brightness;
    OUT[2] = b * brightness;
    
    OUT[3] = 255.0f;
}


void model_frag_shader(vec4 OUT, vec4 IN, vec2 uv, vec3 position,
                          vec3 normal) { 
    OUT[0] = position[0] * IN[0];
    OUT[1] = position[1] * IN[1];
    OUT[2] = position[2] * IN[2];

    OUT[3] = IN[3];
}

camera main_camera = {
    .near = 0.01f,
    .far = 150.0f,
    .fovy = 75.0f,
    .position = {0.0f, 0.0f, 0.0f},
    .rotation = {0.0f, 0.0f, 0.0f},
};

model test_model;
model terrain;

void init_game() {
  main_player.cam = &main_camera;
  main_player.position[0] = 0.0f;
  main_player.position[1] = -7.0f;
  main_player.position[2] = -7.0f;


  init_model(&terrain, NULL,
             (vec3){-15.0f, 0.0f, -15.0f},
             (vec3){0.0f, 0.0f, 0.0f},
             (vec3){1.0f, 1.0f, 1.0f},
             SHAPE_TERRAIN);

  init_model(&test_model, NULL,
             (vec3){0.0f, -1.5f, 5.0f},
             (vec3){0.0f, 0.0f, 0.0f},
             (vec3){1.0f, 1.0f, 1.0f},
             SHAPE_CUBE);
}

void update_game(double deltatime, SDL_Event event) {
  update_player_controller(&main_player, deltatime, event);
}

void update_graphics(SDL_display *display) {
  clear_display(display, 15, 20, 45);

  main_camera.rotation[0] = 0.0f;
  test_model.rotation[1] += 0.5f;

  render_model(display, &terrain, main_player.cam, false, terrain_geo_shader, terrain_frag_shader);
  render_model(display, &test_model, main_player.cam, false, model_geo_shader, model_frag_shader);
}
