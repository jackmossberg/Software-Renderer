#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "display.h"

#define true 1
#define false 0

typedef struct SDL_app {
  SDL_display *display;
  const char *name;

  void (*update_display)(SDL_display *);
  void (*update_gameloop)(double, SDL_Event);
  void (*init_gameloop)();
} SDL_app;

SDL_app *allocate_app(uint16_t width, uint16_t height, const char *title,
                      const char *name, void (*update_display)(SDL_display *),
                      void (*update_gameloop)(double, SDL_Event),
                      void (*init_gameloop)());
void deallocate_app(SDL_app *app);
void update_app(SDL_app *app);
