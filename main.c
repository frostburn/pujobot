#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

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

void play_demo() {
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
    size_t move = maxDropletStrategy3(&g, bag, 2*horizon, &heuristic_score);
    play_simple(&g, bag, move);
    advance_bag(bag, horizon);
    score += resolve_simple(&g);
    print_screen(&(g.screen));
    printf("Score: %d\n", score);
  }

  free(bag);
}

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
}


int main() {
  srand(time(0));

  // benchmark();
  // play_demo();
  test_effective_lockout();

  return EXIT_SUCCESS;
}
