#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "util.c"

#include "bitboard.c"  // lol

#include "screen.c"  // very modular, yes

#include "game.c"  // such a nice linear progression, no?

#include "ai.c"

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

  free(bag);
}

int main() {
  srand(time(0));

  // benchmark();

  simple_game g;
  clear_simple_game(&g);
  color_t *bag = malloc(2 * sizeof(color_t));

  int score = 0;
  for (int i = 0; i < 200; ++i) {
    double heuristic_score;
    bag[0] = g.color_selection[rand() % COLOR_SELECTION_SIZE];
    bag[1] = g.color_selection[rand() % COLOR_SELECTION_SIZE];
    size_t move = maxDropletStrategy1(&g, bag, &heuristic_score);
    play_simple(&g, bag, move);
    score += resolve_simple(&g);
    print_screen(&(g.screen));
    printf("Score: %d\n", score);
  }

  free(bag);

  return EXIT_SUCCESS;
}
