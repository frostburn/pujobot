#include <assert.h>

#include "jkiss/jkiss.h"

#include "pujobot/util.h"
#include "pujobot/bitboard.h"
#include "pujobot/screen.h"
#include "pujobot/game.h"
#include "pujobot/ai.h"

void test_effective_lockout() {
  color_t BAG[2] = {RED, GREEN};
  color_t MONOBAG[2] = {RED, RED};
  simple_game g;
  clear_simple_game(&g);
  int num_puyos = 0;
  for (int y = VISIBLE_HEIGHT-1; y >= 0; --y) {
    for (int x = 0; x < WIDTH; ++x) {
      if (num_puyos >= WIDTH * VISIBLE_HEIGHT - 3) {
        break;
      }
      insert_puyo(&(g.screen), x, y + GHOST_Y + 1, (x+y) % 4);
      num_puyos++;
    }
  }
  assert(effective_lockout(&g, MONOBAG, 2) == 0);
  assert(effective_lockout(&g, BAG, 2) == 0);
  assert(effective_lockout(&g, NULL, 0) == 0);

  insert_puyo(&(g.screen), 3, GHOST_Y + 1, 3);
  assert(effective_lockout(&g, MONOBAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, BAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, NULL, 0) == SIMPLE_GAME_OVER);

  insert_puyo(&(g.screen), 4, GHOST_Y + 1, 0);
  assert(effective_lockout(&g, MONOBAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, BAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, NULL, 0) == SIMPLE_GAME_OVER);

  insert_puyo(&(g.screen), 5, GHOST_Y + 1, 1);
  assert(effective_lockout(&g, MONOBAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, BAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, NULL, 0) == SIMPLE_GAME_OVER);

  g.screen.grid[RED][0] ^= 1 << (GHOST_Y + 1);
  g.screen.grid[RED][4] ^= 1 << (GHOST_Y + 1);
  MONOBAG[0] = GREEN;
  MONOBAG[1] = GREEN;
  assert(effective_lockout(&g, MONOBAG, 2) == SIMPLE_GAME_OVER);
  assert(effective_lockout(&g, NULL, 0) == SIMPLE_GAME_OVER);

  g.screen.grid[RED][0] |= 1 << (GHOST_Y + 1);
  g.screen.grid[BLUE][3] ^= 1 << (GHOST_Y + 1);
  g.screen.grid[GREEN][3] ^= 1 << (GHOST_Y + 1);

  assert(effective_lockout(&g, BAG, 2) == 0);
  assert(effective_lockout(&g, NULL, 0) == 0);
}

int main() {
  jkiss_init();

  test_effective_lockout();

  return EXIT_SUCCESS;
}
