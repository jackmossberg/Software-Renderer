#include "game.h"
#include <stdio.h>

int main() {    
  SDL_app *app =
      allocate_app(DEFAULT_BUFFER_WIDTH, DEFAULT_BUFFER_HEIGHT, "test build",
                   "main", update_graphics, update_game, init_game);
  init_game();
  update_app(app);

  deallocate_app(app);
  return 0;
}
