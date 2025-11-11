#include "display.h"

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
  return display;
}

void deallocate_display(SDL_display *display) {
  SDL_DestroyWindow(display->pointer);
  free(display);
}

void cycle_display(SDL_display *display) {
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
               vec3 v1, vec3 v2, vec3 v3, vec3 pos, vec3 rot, int debug) {
  draw_tri3d_to_backbuffer(display->surface, c, v1, v2, v3, r, g, b, pos, rot, debug);
}

void clear_display(SDL_display *display, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t color = SDL_MapRGB(display->surface->format, r, g, b);
  SDL_FillRect(display->surface, NULL, color);
  SDL_UnlockSurface(display->surface);
}
