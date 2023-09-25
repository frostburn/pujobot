#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

#include "bitboard.c"  // lol

#include "screen.c"  // very modular, yes

#include "game.c"  // such a nice linear progression, no?

void benchmark() {
  simple_game g;
  clear_simple_game(&g);
  color_t *bag = malloc(2 * sizeof(color_t));
  bag[0] = RED;
  bag[1] = GREEN;

  double score = 0;
  size_t num_runs = 100000;
  for (size_t j = 0; j < num_runs; ++j) {
    clear_simple_game(&g);
    for (int i = 0; i < 36; ++i) {
      play_simple(&g, bag, rand() % SIZEOF(MOVES));
      score += resolve_simple(&g);
      // print_screen(&(g.screen));
      bag[0] = rand() % NUM_PUYO_COLORS;
      bag[1] = rand() % NUM_PUYO_COLORS;
    }
  }
  printf("Total score from %zu runs: %g\n", num_runs, score);
}


int main() {
  srand(time(0));

  benchmark();
  return EXIT_SUCCESS;
}
