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
  for (int i = 0; i < MAX_TRI_COUNT; ++i) {
    float x1 = model->tris[i].v1[0];
    float y1 = model->tris[i].v1[1];
    float z1 = model->tris[i].v1[2];
    float x2 = model->tris[i].v2[0];
    float y2 = model->tris[i].v2[1];
    float z2 = model->tris[i].v2[2];
    float x3 = model->tris[i].v3[0];
    float y3 = model->tris[i].v3[1];
    float z3 = model->tris[i].v3[2];

    if (x1 == 0.0f && y1 == 0.0f && z1 == 0.0f && x2 == 0.0f && y2 == 0.0f &&
        z2 == 0.0f && x3 == 0.0f && y3 == 0.0f && z3 == 0.0f)
      continue;

    float ex1 = x2 - x1;
    float ey1 = y2 - y1;
    float ez1 = z2 - z1;
    float ex2 = x3 - x1;
    float ey2 = y3 - y1;
    float ez2 = z3 - z1;

    float nx = ey1 * ez2 - ez1 * ey2;
    float ny = ez1 * ex2 - ex1 * ez2;
    float nz = ex1 * ey2 - ey1 * ex2;

    float cx = (x1 + x2 + x3) / 3.0f;
    float cy = (y1 + y2 + y3) / 3.0f;
    float cz = (z1 + z2 + z3) / 3.0f;

    float dot = nx * cx + ny * cy + nz * cz;
    if (dot > 0.0f) {
      float tx = model->tris[i].v2[0];
      float ty = model->tris[i].v2[1];
      float tz = model->tris[i].v2[2];
      model->tris[i].v2[0] = model->tris[i].v3[0];
      model->tris[i].v2[1] = model->tris[i].v3[1];
      model->tris[i].v2[2] = model->tris[i].v3[2];
      model->tris[i].v3[0] = tx;
      model->tris[i].v3[1] = ty;
      model->tris[i].v3[2] = tz;
    }
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
    .position = {0.0, 0.0, -7.0},
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
  clear_display(display, 15, 20, 30);

  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);

  main_camera.position[0] =
      ((float)mouseX / (float)display->window_width) * 10.0f - 5.0f;
  main_camera.position[1] =
      ((float)mouseY / (float)display->window_height) * 10.0f - 5.0f;

  // test_model.rotation[0] = angle;
  test_model.rotation[1] = angle;
  // test_model.rotation[2] = angle;

  render_model(display, &test_model, &main_camera, false);
}
