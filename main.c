#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// gcc main.c -fopenmp -Ofast -march=native
// gcc -shared -o pujolib.so -fopenmp -fPIC -Ofast -march=native main.c

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

void advance_bag(color_t *bag, size_t horizon) {
  for (size_t i = 0; i < horizon - 1; ++i) {
    bag[2*i+0] = bag[2*i+2];
    bag[2*i+1] = bag[2*i+3];
  }
  bag[2*(horizon-1)+0] = rand() % COLOR_SELECTION_SIZE;
  bag[2*(horizon-1)+1] = rand() % COLOR_SELECTION_SIZE;
}

int main() {
  srand(time(0));

  // benchmark();

  simple_game g;
  clear_simple_game(&g);
  size_t horizon = 3;
  color_t *bag = malloc(2 * horizon * sizeof(color_t));

  for (size_t i = 0; i < horizon; ++i) {
    advance_bag(bag, horizon);
  }

  int score = 0;
  for (int i = 0; i < 200; ++i) {
    double heuristic_score;
    size_t move = maxDropletStrategy3(&g, bag, &heuristic_score);
    play_simple(&g, bag, move);
    advance_bag(bag, horizon);
    score += resolve_simple(&g);
    print_screen(&(g.screen));
    printf("Score: %d\n", score);
  }

  free(bag);

  return EXIT_SUCCESS;
}
