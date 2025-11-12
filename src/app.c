#include "app.h"

#define VARIFYHEAP(pointer, str, type)                                         \
  if (pointer == NULL) {                                                       \
    printf("Heap allocation error: %s\n", str);                                \
    return type;                                                               \
  }

SDL_app *allocate_app(uint16_t width, uint16_t height, const char *title,
                      const char *name, void (*update_display)(SDL_display *),
                      void (*update_gameloop)(double, SDL_Event),
                      void (*init_gameloop)()) {
  SDL_app *app = (SDL_app *)calloc(1, sizeof(SDL_app));
  VARIFYHEAP(app, "allocate_app()", NULL)

  app->name = name;
  app->display = allocate_display(width, height, title);
  app->update_display = update_display;
  app->update_gameloop = update_gameloop;
  app->init_gameloop = init_gameloop;

  return app;
}

void deallocate_app(SDL_app *app) {
  deallocate_display(app->display);
  free(app);

  SDL_Quit();
}

void update_app(SDL_app *app) {
  VARIFYHEAP(app->update_display, "update_app()", )

  app->init_gameloop();

  SDL_Event event;
  int running = 1;
  double deltat = 0.0;
  Uint64 last_time = 0;

  int first_iter = true;
  while (running) {
    app->update_gameloop(deltat, event);
    app->update_display(app->display);
    cycle_display(app->display);

    SDL_PollEvent(&event);

    if (first_iter == false) {
      Uint64 current_time = SDL_GetPerformanceCounter();
      deltat = (double)(current_time - last_time) /
               (double)SDL_GetPerformanceFrequency();
      last_time = current_time;
    }
    first_iter = false;

    if (deltat > 5.0)
      deltat = 0.0;

    if (SCREEN_TICKS_PER_FRAME > deltat) {
      SDL_Delay(SCREEN_TICKS_PER_FRAME - deltat);
    }

    if (event.type == SDL_QUIT) {
      running = 0;
    }
  }
}
