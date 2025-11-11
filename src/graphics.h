#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];
typedef int vec2i[2];
typedef int vec3i[3];
typedef int vec4i[4];
typedef float mat3[3][3];
typedef float mat4[4][4];

typedef struct {
  vec3 position;
  vec3 rotation;
  float fovy;
  float near;
  float far;
} camera;

typedef struct {
  vec2i min;
  vec2i max;
} bbox2i;

typedef struct {
  vec3 p;
  float w;
} clip_vertex;

void update_view_matrix(mat4 *mat, camera c);
void update_model_matrix(mat4 *mat, vec3 pos, vec3 rot);
void update_projection_matrix(mat4 *mat, camera c, uint16_t width,
                              uint16_t height);
void draw_line_to_backbuffer(SDL_Surface *surface, uint8_t r, uint8_t g,
                             uint8_t b, uint16_t x1, uint16_t y1, uint16_t x2,
                             uint16_t y2);
void draw_wireframe_tri_to_backbuffer(SDL_Surface *surface, vec2i v1, vec2i v2,
                                      vec2i v3, uint8_t r, uint8_t g, uint8_t b,
                                      int debug);
void draw_tri_to_backbuffer(SDL_Surface *surface, vec2i v1, vec2i v2, vec2i v3,
                            uint8_t r, uint8_t g, uint8_t b, int debug);
void draw_tri3d_to_backbuffer(SDL_Surface *surface, camera c, vec3 v1, vec3 v2,
                              vec3 v3, uint8_t r, uint8_t g, uint8_t b,
                              vec3 pos, vec3 rot, int debug);
