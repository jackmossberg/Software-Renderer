#include "game.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  SDL_app *app =
      allocate_app(DEFAULT_BUFFER_WIDTH, DEFAULT_BUFFER_HEIGHT, "test build",
                   "main", update_graphics, update_game, init_game);
  init_game();
  update_app(app);

  deallocate_app(app);
  return 0;
}
