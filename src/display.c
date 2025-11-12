#include "display.h"

#define PIXEL_I(x, y) ((y) * DEFAULT_BUFFER_WIDTH + (x))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

#define VARIFYHEAP(pointer, str, type)                                         \
  if (pointer == NULL) {                                                       \
    printf("Heap allocation error: %s\n", str);                                \
    return type;                                                               \
  }

SDL_display *allocate_display(uint16_t width, uint16_t height,
                              const char *title) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    SDL_Log("SDL Failure: %s", SDL_GetError());

  SDL_display *display = (SDL_display *)calloc(1, sizeof(SDL_display));
  VARIFYHEAP(display, "allocate_display()", NULL)

  SDL_GL_SetSwapInterval(1);

  display->window_width = width;
  display->window_height = height;
  display->buffer_width = width / DEFAULT_BUFFER_SCALE_FACTOR;
  display->buffer_height = height / DEFAULT_BUFFER_SCALE_FACTOR;
  display->title = title;

  display->pointer = SDL_CreateWindow(
      display->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      display->window_width, display->window_height, SDL_WINDOW_SHOWN);

  memset(&display->zbuffer, 0, sizeof(display->zbuffer));

  if (!display->pointer) {
    SDL_Log("SDL Window Failure: %s", SDL_GetError());
    SDL_Quit();
    return NULL;
  }

  display->surface = SDL_CreateRGBSurface(
      0, display->buffer_width, display->buffer_height, 32, 0, 0, 0, 0);
  SDL_Surface *frontbuffer = SDL_GetWindowSurface(display->pointer);
  SDL_Rect dst_rect = {0, 0, frontbuffer->w, frontbuffer->h};
  SDL_BlitScaled(display->surface, NULL, frontbuffer, &dst_rect);

  SDL_UpdateWindowSurface(display->pointer);

  for (uint32_t i = 0; i < DEFAULT_BUF_LEN; i++) {
    display->zbuffer.value[i] = 0xFFFFFFFF;
  }

  return display;
}

void deallocate_display(SDL_display *display) {
  VARIFYHEAP(display, "deallocate_display", )
  SDL_DestroyWindow(display->pointer);
  SDL_Quit();
  free(display);
}

void cycle_display(SDL_display *display) {
  if (SDL_MUSTLOCK(display->surface))
    SDL_UnlockSurface(display->surface);

  SDL_Surface *frontbuffer = SDL_GetWindowSurface(display->pointer);
  SDL_Rect dst_rect = {0, 0, frontbuffer->w, frontbuffer->h};
  SDL_BlitScaled(display->surface, NULL, frontbuffer, &dst_rect);
  SDL_UpdateWindowSurface(display->pointer);
}

void set_pixel(SDL_display *display, uint16_t x, uint16_t y, uint8_t r,
               uint8_t g, uint8_t b) {
  if (x < 0 || x >= display->surface->w || y < 0 || y >= display->surface->h)
    return;
  uint32_t value = SDL_MapRGB(display->surface->format, r, g, b);
  uint32_t *pixels = (uint32_t *)display->surface->pixels;

  pixels[y * display->surface->pitch / 4 + x] = value;
}

void set_line(SDL_display *display, uint8_t r, uint8_t g, uint8_t b,
              uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  draw_line_to_backbuffer(display->surface, r, g, b, x1, y1, x2, y2);
}

void set_wframe_tri(SDL_display *display, uint8_t r, uint8_t g, uint8_t b,
                    vec2i v1, vec2i v2, vec2i v3, int debug) {
  draw_wireframe_tri_to_backbuffer(display->surface, v1, v2, v3, r, g, b,
                                   debug);
}
void set_tri(SDL_display *display, uint8_t r, uint8_t g, uint8_t b, vec2i v1,
             vec2i v2, vec2i v3, int debug) {
  draw_tri_to_backbuffer(display->surface, v1, v2, v3, r, g, b, debug);
}

void set_tri3d(SDL_display *display, camera c, uint8_t r, uint8_t g, uint8_t b,
               vec3 v1, vec3 v2, vec3 v3, vec3 pos, vec3 rot, vec3 pivot,
               int debug, void (*shader)(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r,
            uint8_t g, uint8_t b)) {
  draw_tri3d_to_backbuffer_zbuffered(display->surface, display->zbuffer.value,
                                     c, v1, v2, v3, r, g, b, pos, rot, pivot,
                                     debug, shader);
}

void set_tri3d_no_zbuffer(SDL_display *display, camera c, uint8_t r, uint8_t g,
                          uint8_t b, vec3 v1, vec3 v2, vec3 v3, vec3 pos,
                          vec3 rot, vec3 pivot, int debug, void (*shader)(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r,
            uint8_t g, uint8_t b)) {
  draw_tri3d_to_backbuffer(display->surface, c, v1, v2, v3, r, g, b, pos, rot,
                           pivot, debug, shader);
}

void clear_display(SDL_display *display, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t color = SDL_MapRGB(display->surface->format, r, g, b);
  if (SDL_MUSTLOCK(display->surface))
    SDL_LockSurface(display->surface);
  SDL_FillRect(display->surface, NULL, color);
  for (uint32_t i = 0; i < DEFAULT_BUF_LEN; i++) {
    display->zbuffer.value[i] = 0xFFFFFFFF;
  }
}

static void init_tris_CUBE(tri *out) {
  static tri cube_tris[12] = {
    {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {0.5f, -0.5f, 0.5f}, .v3 = {0.5f, 0.5f, 0.5f}},
    {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {0.5f, 0.5f, 0.5f}, .v3 = {-0.5f, 0.5f, 0.5f}},
    
    {.v1 = {0.5f, -0.5f, -0.5f}, .v2 = {-0.5f, -0.5f, -0.5f}, .v3 = {-0.5f, 0.5f, -0.5f}},
    {.v1 = {0.5f, -0.5f, -0.5f}, .v2 = {-0.5f, 0.5f, -0.5f}, .v3 = {0.5f, 0.5f, -0.5f}},
    
    {.v1 = {-0.5f, 0.5f, -0.5f}, .v2 = {-0.5f, 0.5f, 0.5f}, .v3 = {0.5f, 0.5f, 0.5f}},
    {.v1 = {-0.5f, 0.5f, -0.5f}, .v2 = {0.5f, 0.5f, 0.5f}, .v3 = {0.5f, 0.5f, -0.5f}},
    
    {.v1 = {-0.5f, -0.5f, -0.5f}, .v2 = {0.5f, -0.5f, -0.5f}, .v3 = {0.5f, -0.5f, 0.5f}},
    {.v1 = {-0.5f, -0.5f, -0.5f}, .v2 = {0.5f, -0.5f, 0.5f}, .v3 = {-0.5f, -0.5f, 0.5f}},
    
    {.v1 = {0.5f, -0.5f, -0.5f}, .v2 = {0.5f, -0.5f, 0.5f}, .v3 = {0.5f, 0.5f, 0.5f}},
    {.v1 = {0.5f, -0.5f, -0.5f}, .v2 = {0.5f, 0.5f, 0.5f}, .v3 = {0.5f, 0.5f, -0.5f}},
    
    {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {-0.5f, -0.5f, -0.5f}, .v3 = {-0.5f, 0.5f, -0.5f}},
    {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {-0.5f, 0.5f, -0.5f}, .v3 = {-0.5f, 0.5f, 0.5f}},
  };
  for (int i = 0; i < 12; i++) {
    out[i] = cube_tris[i];
  }
}

static void init_tris_PYRAMID(tri *out) {
  static tri pyramid_tris[6] = {
    {.v1 = {-0.5f, -0.5f, -0.5f}, .v2 = {0.5f, -0.5f, -0.5f}, .v3 = {0.5f, -0.5f, 0.5f}},
    {.v1 = {-0.5f, -0.5f, -0.5f}, .v2 = {0.5f, -0.5f, 0.5f}, .v3 = {-0.5f, -0.5f, 0.5f}},
    
    {.v1 = {-0.5f, -0.5f, 0.5f}, .v2 = {0.5f, -0.5f, 0.5f}, .v3 = {0.0f, 0.5f, 0.0f}},

    {.v1 = {0.5f, -0.5f, 0.5f}, .v2 = {0.5f, -0.5f, -0.5f}, .v3 = {0.0f, 0.5f, 0.0f}},
    
    {.v1 = {0.5f, -0.5f, -0.5f}, .v2 = {-0.5f, -0.5f, -0.5f}, .v3 = {0.0f, 0.5f, 0.0f}},
    
    {.v1 = {-0.5f, -0.5f, -0.5f}, .v2 = {-0.5f, -0.5f, 0.5f}, .v3 = {0.0f, 0.5f, 0.0f}},
  };
  for (int i = 0; i < 6; i++) {
    out[i] = pyramid_tris[i];
  }
}

static void init_tris_ICO_SPHERE(tri *out) {
  float phi = (1.0f + sqrtf(5.0f)) * 0.5f;
  //float inv_phi = 1.0f / phi;
  
  vec3 vertices[12] = {
    {-1.0f,  phi,  0.0f},
    { 1.0f,  phi,  0.0f},
    {-1.0f, -phi,  0.0f},
    { 1.0f, -phi,  0.0f},

    { 0.0f, -1.0f,  phi},
    { 0.0f,  1.0f,  phi},
    { 0.0f, -1.0f, -phi},
    { 0.0f,  1.0f, -phi},
    
    { phi,  0.0f, -1.0f},
    { phi,  0.0f,  1.0f},
    {-phi,  0.0f, -1.0f},
    {-phi,  0.0f,  1.0f},
  };
  
  for (int i = 0; i < 12; i++) {
    float len = sqrtf(vertices[i][0] * vertices[i][0] + 
                      vertices[i][1] * vertices[i][1] + 
                      vertices[i][2] * vertices[i][2]);
    if (len > 0.0f) {
      vertices[i][0] /= len;
      vertices[i][1] /= len;
      vertices[i][2] /= len;
    }
  }
  
  int indices[20][3] = {
    {0, 11, 5},
    {0, 5, 1},
    {0, 1, 7},
    {0, 7, 10},
    {0, 10, 11},
    
    {1, 5, 9},
    {5, 11, 4},
    {11, 10, 2},
    {10, 7, 6},
    {7, 1, 8},

    {3, 9, 4},
    {3, 4, 2},
    {3, 2, 6},
    {3, 6, 8},
    {3, 8, 9},
    
    {4, 9, 5},
    {2, 4, 11},
    {6, 2, 10},
    {8, 6, 7},
    {9, 8, 1},
  };
  
  for (int i = 0; i < 20; i++) {
    out[i].v1[0] = vertices[indices[i][0]][0];
    out[i].v1[1] = vertices[indices[i][0]][1];
    out[i].v1[2] = vertices[indices[i][0]][2];
    
    out[i].v2[0] = vertices[indices[i][1]][0];
    out[i].v2[1] = vertices[indices[i][1]][1];
    out[i].v2[2] = vertices[indices[i][1]][2];
    
    out[i].v3[0] = vertices[indices[i][2]][0];
    out[i].v3[1] = vertices[indices[i][2]][1];
    out[i].v3[2] = vertices[indices[i][2]][2];
  }
}

static void init_tris_TERRAIN(tri *out) {
  int xsize = 20;
  int zsize = 20;
  float scale = 1.5f;

  for (int x = 0; x < xsize - 1; x++) {
    for (int z = 0; z < zsize - 1; z++) {
      int index = (x * (zsize - 1) + z) * 2;

      out[index].v1[0] = x * scale;
      out[index].v1[1] = 0.0f;
      out[index].v1[2] = z * scale;
      
      out[index].v2[0] = x * scale;
      out[index].v2[1] = 0.0f;
      out[index].v2[2] = (z + 1) * scale;
      
      out[index].v3[0] = (x + 1) * scale;
      out[index].v3[1] = 0.0f;
      out[index].v3[2] = z * scale;
      
      out[index + 1].v1[0] = (x + 1) * scale;
      out[index + 1].v1[1] = 0.0f;
      out[index + 1].v1[2] = z * scale;
      
      out[index + 1].v2[0] = x * scale;
      out[index + 1].v2[1] = 0.0f;
      out[index + 1].v2[2] = (z + 1) * scale;
      
      out[index + 1].v3[0] = (x + 1) * scale;
      out[index + 1].v3[1] = 0.0f;
      out[index + 1].v3[2] = (z + 1) * scale;
    }
  }
}

void init_model(model *model, tri *tris, vec3 position, vec3 rotation,
                vec3 scale, int SHAPE) {
  if (tris == NULL) {
    tri temp_tris[MAX_TRI_COUNT];
    for (int i = 0; i < MAX_TRI_COUNT; i++) {
      temp_tris[i].v1[0] = temp_tris[i].v1[1] = temp_tris[i].v1[2] = 0.0f;
      temp_tris[i].v2[0] = temp_tris[i].v2[1] = temp_tris[i].v2[2] = 0.0f;
      temp_tris[i].v3[0] = temp_tris[i].v3[1] = temp_tris[i].v3[2] = 0.0f;
    }
    
    switch (SHAPE)
    {
    case SHAPE_CUBE:
      init_tris_CUBE(temp_tris);
      break;
    
    case SHAPE_PYRAMID:
      init_tris_PYRAMID(temp_tris);
      break;
    
    case SHAPE_ICO_SPHERE:
      init_tris_ICO_SPHERE(temp_tris);
      break;

    case SHAPE_TERRAIN:
      init_tris_TERRAIN(temp_tris);
      break;

    default:
      break;
    }
    
    for (int i = 0; i < MAX_TRI_COUNT; i++) {
      model->tris[i] = temp_tris[i];
    }
  } else {
    for (int i = 0; i < MAX_TRI_COUNT; i++) {
      model->tris[i] = tris[i];
    }
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
  model->shader = NULL;

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

void render_model(SDL_display *display, model *m, camera *c, int wframe, void (*shader)(vec4 OUT, vec3 normal, vec2 uv, vec3 position, vec3 light_dir, uint8_t r,
            uint8_t g, uint8_t b)) {
  for (int i = 0; i < MAX_TRI_COUNT; i++) {
    if (m->tris[i].v1[0] == 0.0f && m->tris[i].v1[1] == 0.0f &&
        m->tris[i].v1[2] == 0.0f && m->tris[i].v2[0] == 0.0f &&
        m->tris[i].v2[1] == 0.0f && m->tris[i].v2[2] == 0.0f &&
        m->tris[i].v3[0] == 0.0f && m->tris[i].v3[1] == 0.0f &&
        m->tris[i].v3[2] == 0.0) {
      continue;
    }
    set_tri3d(display, *c, 255, 255, 255, m->tris[i].v1, m->tris[i].v2,
              m->tris[i].v3, m->position, m->rotation,
              (vec3){0.0, 0.0f, 0.0f}, wframe, shader);
  }
}