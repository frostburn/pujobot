#include <time.h>

#include "jkiss/jkiss.h"

#include "pujobot/util.h"
#include "pujobot/bitboard.h"
#include "pujobot/screen.h"
#include "pujobot/game.h"
#include "pujobot/ai.h"

void advance_bag(color_t *bag, size_t horizon) {
  for (size_t i = 0; i < horizon - 1; ++i) {
    bag[2*i+0] = bag[2*i+2];
    bag[2*i+1] = bag[2*i+3];
  }
  bag[2*(horizon-1)+0] = rand() % COLOR_SELECTION_SIZE;
  bag[2*(horizon-1)+1] = rand() % COLOR_SELECTION_SIZE;
}

void play_demo() {
  simple_game g;
  clear_simple_game(&g);
  g.screen.jkiss = jkiss32_spawn();
  size_t horizon = 3;
  color_t *bag = calloc(2 * horizon, sizeof(color_t));

  for (size_t i = 0; i < horizon; ++i) {
    advance_bag(bag, horizon);
  }

  int score = 0;
  for (int i = 0; i < 200; ++i) {
    if (rand() < RAND_MAX / 12) {
      if (rand() < RAND_MAX / 2) {
        g.pending_garbage += (rand() % 25) + (rand() % 25);
      } else {
        g.late_garbage += (rand() % 25) + (rand() % 25);
        g.late_time_remaining = rand() % 100;
      }
    }
    double heuristic_score;
    move_t move = flex_droplet_strategy_3(&g, bag, 2*horizon, &heuristic_score);
    play_simple(&g, bag, move);
    advance_bag(bag, horizon);
    int move_score = resolve_simple(&g);
    score += move_score;
    print_simple_game(&g);
    printf("Score: %d\n", score);
    if (move_score < 0) {
      clear_simple_game(&g);
    }
  }

  free(bag);
}

int main() {
  srand(time(0));
  jkiss_init();

  play_demo();

  return EXIT_SUCCESS;
}
