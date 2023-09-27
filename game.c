#define ONE_STONE (30)
#define COLOR_SELECTION_SIZE (4)
// Value all-clears based on the amount of garbage they send.
#define SIMPLE_ALL_CLEAR_BONUS (2100)
// Not even a 19-chain can compensate a Game Over.
#define SIMPLE_GAME_OVER (-1000000)

// x1, y1, x2, y2, orientation
static const size_t MOVES[][5] = {
  {0, 2, 0, 1, 0},
  {1, 2, 1, 1, 0},
  {2, 2, 2, 1, 0},
  {3, 2, 3, 1, 0},
  {4, 2, 4, 1, 0},
  {5, 2, 5, 1, 0},
  // Orientation = 1
  {1, 1, 0, 1, 1},
  {2, 1, 1, 1, 1},
  {3, 1, 2, 1, 1},
  {4, 1, 3, 1, 1},
  {5, 1, 4, 1, 1},
  // Orientation = 2
  {0, 1, 0, 2, 2},
  {1, 1, 1, 2, 2},
  {2, 1, 2, 2, 2},
  {3, 1, 3, 2, 2},
  {4, 1, 4, 2, 2},
  {5, 1, 5, 2, 2},
  // Orientation = 3
  {0, 1, 1, 1, 3},
  {1, 1, 2, 1, 3},
  {2, 1, 3, 1, 3},
  {3, 1, 4, 1, 3},
  {4, 1, 5, 1, 3},
};

typedef struct simple_game {
  simple_screen screen;
  int pending_garbage;
  int late_garbage;
  float late_time_remaining;
  float move_time;
  color_t color_selection[COLOR_SELECTION_SIZE];
} simple_game;

void clear_simple_game(simple_game *g) {
  clear_simple_screen(&(g->screen));
  g->pending_garbage = 0;
  g->late_garbage = 0;
  g->late_time_remaining = 0;
  g->move_time = 0.3846;
  g->color_selection[0] = 0;
  g->color_selection[1] = 1;
  g->color_selection[2] = 2;
  g->color_selection[3] = 3;
}

size_t get_simple_moves(simple_game *g, color_t *bag, size_t *moves_out) {
  puyos mask;
  store_mask(mask, g->screen.grid);
  bool symmetric = bag != NULL && bag[0] == bag[1];
  size_t num_moves = 0;
  for (size_t i = 0; i < SIZEOF(MOVES); ++i) {
    if (symmetric && 2*i >= SIZEOF(MOVES)) {
      return num_moves;
    }
    size_t x1 = MOVES[i][0];
    size_t x2 = MOVES[i][2];
    if (!puyo_at(mask, x1, GHOST_Y) || !puyo_at(mask, x2, GHOST_Y)) {
      moves_out[num_moves++] = i;
    }
  }
  return num_moves;
}

void play_simple(simple_game *g, color_t *bag, size_t move_index) {
  color_t color1 = bag[0];
  color_t color2 = bag[1];

  insert_puyo(&(g->screen), MOVES[move_index][0], MOVES[move_index][1], color1);
  insert_puyo(&(g->screen), MOVES[move_index][2], MOVES[move_index][3], color2);
  
  int releasedGarbage = g->pending_garbage;
  if (releasedGarbage > ONE_STONE) {
    releasedGarbage = ONE_STONE;
  }
  g->pending_garbage -= releasedGarbage;
  g->screen.buffered_garbage += releasedGarbage;
}

int resolve_simple(simple_game *g) {
  int chain_number;
  int score = tick_simple_screen(&(g->screen), &chain_number);
  
  g->late_time_remaining -= chain_number + g->move_time;
  if (g->late_time_remaining <= 0) {
    g->pending_garbage += g->late_garbage;
    g->late_garbage = 0;
  }

  if (is_all_clear(&(g->screen))) {
    score += SIMPLE_ALL_CLEAR_BONUS;
  }
  if (is_locked_out(&(g->screen))) {
    score += SIMPLE_GAME_OVER;
  }
  return score;
}

void print_simple_game(simple_game *g) {
  print_screen(&(g->screen));
  printf("Pending garbage = %d\n", g->pending_garbage);
  printf("Late garbage = %d @ %g\n", g->late_garbage, g->late_time_remaining);
  printf("Move time = %g\n", g->move_time);
  printf("Color selection = [%d, %d, %d, %d]\n", g->color_selection[0], g->color_selection[1], g->color_selection[2], g->color_selection[3]);
}
