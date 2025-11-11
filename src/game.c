#include "game.h"

#include <limits.h>

#define UNUSED FLOAT_MIN
#define MAX_TRI_COUNT 150

typedef struct tri {
  vec3 v1;
  vec3 v2;
  vec3 v3;
} tri;

typedef struct model {
  vec3 position;
  vec3 rotation;
  vec3 scale;

  tri tris[MAX_TRI_COUNT];
} model;

void init_model(model *model, tri *tris, vec3 position, vec3 rotation,
                vec3 scale) {
  for (int i = 0; i < MAX_TRI_COUNT; i++) {
    model->tris[i] = tris[i];
  }
  model->position[0] = position[0];
  model->position[1] = position[1];
  model->position[2] = position[2];
  model->rotation[0] = rotation[0];
  model->rotation[1] = rotation[1];
  model->rotation[2] = rotation[2];
  model->scale[0] = scale[0];
  model->scale[1] = scale[1];
  model->scale[2] = scale[2];

  for (int i = 0; i < MAX_TRI_COUNT; i++) {
    model->tris[i].v1[0] *= model->scale[0];
    model->tris[i].v1[1] *= model->scale[1];
    model->tris[i].v1[2] *= model->scale[2];
    model->tris[i].v2[0] *= model->scale[0];
    model->tris[i].v2[1] *= model->scale[1];
    model->tris[i].v2[2] *= model->scale[2];
    model->tris[i].v3[0] *= model->scale[0];
    model->tris[i].v3[1] *= model->scale[1];
    model->tris[i].v3[2] *= model->scale[2];
  }
}

void render_model(SDL_display *display, model *m, camera *c, int wframe) {
  for (int i = 0; i < MAX_TRI_COUNT; i++) {
    if (m->tris[i].v1[0] == 0.0f && m->tris[i].v1[1] == 0.0f &&
        m->tris[i].v1[2] == 0.0f && m->tris[i].v2[0] == 0.0f &&
        m->tris[i].v2[1] == 0.0f && m->tris[i].v2[2] == 0.0f &&
        m->tris[i].v3[0] == 0.0f && m->tris[i].v3[1] == 0.0f &&
        m->tris[i].v3[2] == 0.0) {
      return;
    } else {
      set_tri3d(display, *c, 255, 100, 0, m->tris[i].v1, m->tris[i].v2,
                m->tris[i].v3, m->position, m->rotation,
                (vec3){0.0, 0.0f, 0.0f}, wframe);
    }
  }
}

float angle = 0.0;
camera main_camera = {
    .near = 0.01f,
    .far = 150.0f,
    .fovy = 90.0f,
    .position = {0.0, 0.0, -8.0},
    .rotation = {0.0, 0.0, 0.0},
};

model test_model;

void init_game() {
  static tri tris[MAX_TRI_COUNT] = {(tri){.v1 = {0.5f, 0.5f, 0.5f},
                                          .v2 = {-0.5f, -0.5f, 0.5f},
                                          .v3 = {-0.5f, 0.5f, 0.5f}},
                                    (tri){.v1 = {0.5f, 0.5f, 0.5f},
                                          .v2 = {0.5f, -0.5f, 0.5f},
                                          .v3 = {-0.5f, -0.5f, 0.5f}},

                                    (tri){.v1 = {-0.5f, 0.5f, -0.5f},
                                          .v2 = {-0.5f, -0.5f, -0.5f},
                                          .v3 = {0.5f, -0.5f, -0.5f}},
                                    (tri){.v1 = {-0.5f, 0.5f, -0.5f},
                                          .v2 = {0.5f, -0.5f, -0.5f},
                                          .v3 = {0.5f, 0.5f, -0.5f}},

                                    (tri){.v1 = {-0.5f, 0.5f, -0.5f},
                                          .v2 = {-0.5f, 0.5f, 0.5f},
                                          .v3 = {0.5f, 0.5f, 0.5f}},
                                    (tri){.v1 = {-0.5f, 0.5f, -0.5f},
                                          .v2 = {0.5f, 0.5f, 0.5f},
                                          .v3 = {0.5f, 0.5f, -0.5f}},

                                    (tri){.v1 = {-0.5f, -0.5f, 0.5f},
                                          .v2 = {-0.5f, -0.5f, -0.5f},
                                          .v3 = {0.5f, -0.5f, -0.5f}},
                                    (tri){.v1 = {-0.5f, -0.5f, 0.5f},
                                          .v2 = {0.5f, -0.5f, -0.5f},
                                          .v3 = {0.5f, -0.5f, 0.5f}},

                                    (tri){.v1 = {0.5f, 0.5f, 0.5f},
                                          .v2 = {0.5f, -0.5f, -0.5f},
                                          .v3 = {0.5f, -0.5f, 0.5f}},
                                    (tri){.v1 = {0.5f, 0.5f, 0.5f},
                                          .v2 = {0.5f, 0.5f, -0.5f},
                                          .v3 = {0.5f, -0.5f, -0.5f}},

                                    (tri){.v1 = {-0.5f, 0.5f, -0.5f},
                                          .v2 = {-0.5f, -0.5f, 0.5f},
                                          .v3 = {-0.5f, -0.5f, -0.5f}},
                                    (tri){.v1 = {-0.5f, 0.5f, -0.5f},
                                          .v2 = {-0.5f, 0.5f, 0.5f},
                                          .v3 = {-0.5f, -0.5f, 0.5f}}};
  init_model(&test_model, tris, (vec3){0.0, 0.0, 0.0}, (vec3){0.0, 0.0, 0.0},
             (vec3){4.0, 4.0, 4.0});
}

void update_game(double deltatime, SDL_Event event) {
  (void)deltatime;
  (void)event;
  angle += (float)deltatime * 20.0;
}

void update_graphics(SDL_display *display) {
  (void)display;
  clear_display(display, 25, 25, 50);

  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);

  test_model.rotation[0] = angle;
  test_model.rotation[1] = angle;
  test_model.rotation[2] = angle;

  render_model(display, &test_model, &main_camera, false);
}
