#include <SDL2/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "graphics.h"

#define SCREEN_FPS 244
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#ifndef DEFAULT_BUFFER_WIDTH
#define DEFAULT_BUFFER_WIDTH 900
#endif
#ifndef DEFAULT_BUFFER_HEIGHT
#define DEFAULT_BUFFER_HEIGHT 800
#endif

#define DEFAULT_BUFFER_SCALE_FACTOR 4

#define DEFAULT_BUF_LEN                                                        \
  (uint32_t)(((DEFAULT_BUFFER_WIDTH / DEFAULT_BUFFER_SCALE_FACTOR) *           \
              (DEFAULT_BUFFER_HEIGHT / DEFAULT_BUFFER_SCALE_FACTOR)))
typedef struct buffer {
  uint32_t value[DEFAULT_BUF_LEN];
} buffer;

typedef struct SDL_display {
  SDL_Window *pointer;

  uint16_t buffer_width;
  uint16_t buffer_height;

  uint16_t window_width;
  uint16_t window_height;

  const char *title;

  SDL_Surface *surface;
  buffer zbuffer;
} SDL_display;

SDL_display *allocate_display(uint16_t width, uint16_t height,
                              const char *title);
void deallocate_display(SDL_display *display);
void cycle_display(SDL_display *display);
void set_pixel(SDL_display *display, uint16_t x, uint16_t y, uint8_t r,
               uint8_t g, uint8_t b);
void set_line(SDL_display *display, uint8_t r, uint8_t g, uint8_t b,
              uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void set_wframe_tri(SDL_display *display, uint8_t r, uint8_t g, uint8_t b,
                    vec2i v1, vec2i v2, vec2i v3, int debug);
void set_tri(SDL_display *display, uint8_t r, uint8_t g, uint8_t b, vec2i v1,
             vec2i v2, vec2i v3, int debug);
void set_tri3d(SDL_display *display, camera c, uint8_t r, uint8_t g, uint8_t b,
               vec3 v1, vec3 v2, vec3 v3, vec3 pos, vec3 rot, vec3 pivot,
               int debug, void (*shader)(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r,
            uint8_t g, uint8_t b));
void clear_display(SDL_display *display, uint8_t r, uint8_t g, uint8_t b);

#define MAX_TRI_COUNT 1024

#define SHAPE_NONE 0
#define SHAPE_CUBE 1
#define SHAPE_PYRAMID 2
#define SHAPE_ICO_SPHERE 3
#define SHAPE_TERRAIN 4

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

  void (*shader)(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r,
            uint8_t g, uint8_t b);
} model;

void init_model(model *model, tri *tris, vec3 position, vec3 rotation,
                vec3 scale, int SHAPE);
void render_model(SDL_display *display, model *m, camera *c, int wframe, void (*shader)(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r, uint8_t g, uint8_t b));