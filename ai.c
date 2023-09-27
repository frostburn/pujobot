#define HEURISTIC_FAIL (-2000000)
#define PREFER_LONGER (1.1)

int maxDroplet(simple_game *g) {
  int max = HEURISTIC_FAIL;
  for (int i = 0; i < COLOR_SELECTION_SIZE; ++i) {
    for (int x = 0; x < WIDTH; ++x) {
      simple_game clone = *g;
      insert_puyo(&(clone.screen), x, 1, g->color_selection[i]);
      int score = resolve_simple(&clone);
      if (score > max) {
        max = score;
      }
    }
  }
  return max;
}

size_t maxDropletStrategy1(simple_game *g, color_t *bag, double *score_out) {
  size_t moves[SIZEOF(MOVES)];
  size_t num_moves = get_simple_moves(g, bag, moves);
  // Shuffle to break ties
  shuffle(moves, num_moves);

  *score_out = HEURISTIC_FAIL;
  size_t move = num_moves ? moves[0] : 0;

  for (size_t i = 0; i < num_moves; ++i) {
    simple_game clone = *g;
    play_simple(&clone, bag, moves[i]);
    int move_score = resolve_simple(&clone);
    double score = move_score + PREFER_LONGER * maxDroplet(&clone);
    if (score > *score_out) {
      *score_out = score;
      move = moves[i];
    }
  }

  return move;
}

size_t maxDropletStrategy2(simple_game *g, color_t *bag, double *score_out) {
  size_t moves[SIZEOF(MOVES)];
  size_t num_moves = get_simple_moves(g, bag, moves);
  // Shuffle to break ties
  shuffle(moves, num_moves);

  *score_out = HEURISTIC_FAIL;
  size_t move = num_moves ? moves[0] : 0;

  for (size_t i = 0; i < num_moves; ++i) {
    simple_game clone = *g;
    play_simple(&clone, bag, moves[i]);
    int move_score = resolve_simple(&clone);
    double search_score;
    maxDropletStrategy1(&clone, bag + 2, & search_score);
    double score = move_score + PREFER_LONGER * search_score;
    if (score > *score_out) {
      *score_out = score;
      move = moves[i];
    }
  }

  return move;
}
