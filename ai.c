#define HEURISTIC_FAIL (-2000000)
#define PREFER_LONGER (1.1)

/**
 * Determine if the game is effectively locked out.
 */
int effective_lockout(simple_game *g, color_t *bag, size_t bag_remaining) {
  puyos p;
  puyos mask;
  store_mask(mask, g->screen.grid);
  keep_visible(mask);
  int count = puyo_count(mask);
  if (count < WIDTH * VISIBLE_HEIGHT - 2) {
    return 0;
  }
  if (count >= WIDTH * VISIBLE_HEIGHT) {
    return SIMPLE_GAME_OVER;
  }
  invert(mask);
  keep_visible(mask);
  if (bag_remaining >= 2) {
    if (bag[0] == bag[1] && is_contiguous2(mask)) {
      store_clone(p, g->screen.grid[bag[0]]);
      keep_visible(p);
      merge(p, mask);
      flood(mask, p);
      if (puyo_count(mask) >= CLEAR_THRESHOLD) {
        return 0;
      }
      return SIMPLE_GAME_OVER;
    } else {
      puyos piece;
      chip(mask, piece);

      puyos group;

      store_clone(p, g->screen.grid[bag[0]]);
      keep_visible(p);
      store_clone(group, mask);
      merge(p, group);
      flood(group, p);
      if (puyo_count(group) >= CLEAR_THRESHOLD) {
        return 0;
      }
      store_clone(group, piece);
      merge(p, group);
      flood(group, p);
      if (puyo_count(group) >= CLEAR_THRESHOLD) {
        return 0;
      }

      store_clone(p, g->screen.grid[bag[1]]);
      keep_visible(p);
      store_clone(group, mask);
      merge(p, group);
      flood(group, p);
      if (puyo_count(group) >= CLEAR_THRESHOLD) {
        return 0;
      }
      store_clone(group, piece);
      merge(p, group);
      flood(group, p);
      if (puyo_count(group) >= CLEAR_THRESHOLD) {
        return 0;
      }

      return SIMPLE_GAME_OVER;
    }
  } else {
    puyos piece1;
    puyos piece2;
    store_clone(piece1, mask);
    chip(piece1, piece2);
    bool contiguous = is_contiguous2(mask);
    for (int i = 0; i < COLOR_SELECTION_SIZE; ++i) {
      puyos group;

      if (contiguous) {
        store_clone(p, g->screen.grid[g->color_selection[i]]);
        keep_visible(p);
        store_clone(group, mask);
        merge(p, group);
        flood(group, p);
        if (puyo_count(group) >= CLEAR_THRESHOLD) {
          return 0;
        }
      }

      store_clone(p, g->screen.grid[g->color_selection[i]]);
      keep_visible(p);
      store_clone(group, piece1);
      merge(p, group);
      flood(group, p);
      if (puyo_count(group) >= CLEAR_THRESHOLD) {
        return 0;
      }

      store_clone(p, g->screen.grid[g->color_selection[i]]);
      keep_visible(p);
      store_clone(group, piece2);
      merge(p, group);
      flood(group, p);
      if (puyo_count(group) >= CLEAR_THRESHOLD) {
        return 0;
      }
    }
    return SIMPLE_GAME_OVER;
  }
}

/**
 * Heuristic score to discourage wasting of material.
 */
int material_count(simple_game *g) {
  puyos mask;
  store_colored_mask(mask, g->screen.grid);
  return puyo_count(mask);
}

/**
 * Heuristic score to discourage top-outs.
 */
int top_penalty(simple_game *g) {
  puyos mask;
  store_mask(mask, g->screen.grid);
  slice_t top_lines = (1 << GHOST_Y) | (1 << (GHOST_Y + 1)) | (1 << (GHOST_Y + 2));

  for (int x = 0; x < NUM_SLICES; ++x) {
    mask[x] &= top_lines;
  }
  int result = puyo_count(mask);

  top_lines ^= 1 << (GHOST_Y + 2);

  for (int x = 0; x < NUM_SLICES; ++x) {
    mask[x] &= top_lines;
  }
  result += puyo_count(mask);

  top_lines ^= 1 << (GHOST_Y + 1);

  for (int x = 0; x < NUM_SLICES; ++x) {
    mask[x] &= top_lines;
  }
  result += puyo_count(mask);

  return -result;
}

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

int flexDroplet(simple_game *g) {
  double sum = 0;
  int true_max = HEURISTIC_FAIL;
  for (int i = 0; i < COLOR_SELECTION_SIZE; ++i) {
    int max = HEURISTIC_FAIL;
    for (int x = 0; x < WIDTH; ++x) {
      simple_game clone = *g;
      insert_puyo(&(clone.screen), x, 1, g->color_selection[i]);
      int score = resolve_simple(&clone);
      if (score > max) {
        max = score;
      }
    }
    sum += max;
    if (max > true_max) {
      true_max = max;
    }
  }
  return 0.8 * sum / COLOR_SELECTION_SIZE + 0.2 * true_max;
}

size_t flexDropletStrategy1(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
  if (bag_remaining < 2) {
    fprintf(stderr, "Flex 1 needs at least a bag of 2.\n");
    exit(EXIT_FAILURE);
  }

  move_t moves[MAX_NUM_MOVES];
  size_t num_moves = get_simple_moves(g, bag, moves);
  // Shuffle to break ties
  shuffle(moves, num_moves);

  double flex_bonus = 0;
  double max = HEURISTIC_FAIL;
  size_t move = num_moves ? moves[0] : 0;

  for (size_t i = 0; i < num_moves; ++i) {
    simple_game clone = *g;
    play_simple(&clone, bag, moves[i]);
    int move_score = 0;
    if (moves[i] != PASS) {
      move_score = resolve_simple(&clone);
    }
    double score = (
      move_score +
      PREFER_LONGER * flexDroplet(&clone) +
      material_count(&clone) +
      top_penalty(&clone) +
      effective_lockout(&clone, bag + 2, bag_remaining - 2)
    );
    if (score > max) {
      max = score;
      move = moves[i];
    }
    flex_bonus += score;
  }
  if (num_moves > 0) {
    flex_bonus /= num_moves;
  }

  *score_out = 0.9 * max + 0.1 * flex_bonus;

  return move;
}

size_t flexDropletStrategy2(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
  if (bag_remaining < 4) {
    fprintf(stderr, "Flex 2 needs at least a bag of 4.\n");
    exit(EXIT_FAILURE);
  }

  move_t moves[MAX_NUM_MOVES];
  size_t num_moves = get_simple_moves(g, bag, moves);
  // Shuffle to break ties
  shuffle(moves, num_moves);

  double flex_bonus = 0;
  double max = HEURISTIC_FAIL;
  size_t move = num_moves ? moves[0] : 0;

  for (size_t i = 0; i < num_moves; ++i) {
    simple_game clone = *g;
    play_simple(&clone, bag, moves[i]);
    int move_score = 0;
    if (moves[i] != PASS) {
      move_score = resolve_simple(&clone);
    }
    double search_score;
    flexDropletStrategy1(&clone, bag + 2, bag_remaining - 2, &search_score);
    double score = move_score + PREFER_LONGER * search_score;
    if (score > max) {
      max = score;
      move = moves[i];
    }
    flex_bonus += score;
  }
  if (num_moves > 0) {
    flex_bonus /= num_moves;
  }

  *score_out = 0.9 * max + 0.1 * flex_bonus;

  return move;
}

size_t flexDropletStrategy3(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
  if (bag_remaining < 6) {
    fprintf(stderr, "Flex 3 needs at least a bag of 6.\n");
    exit(EXIT_FAILURE);
  }

  move_t moves[MAX_NUM_MOVES];
  size_t num_moves = get_simple_moves(g, bag, moves);
  // Shuffle to break ties
  shuffle(moves, num_moves);

  double flex_bonus = 0;
  double max = HEURISTIC_FAIL;
  size_t move = num_moves ? moves[0] : 0;

  double scores[MAX_NUM_MOVES];

  #pragma omp parallel for
  for (size_t i = 0; i < num_moves; ++i) {
    simple_game clone = *g;
    play_simple(&clone, bag, moves[i]);
    int move_score = 0;
    if (moves[i] != PASS) {
      move_score = resolve_simple(&clone);
    }
    double search_score;
    flexDropletStrategy2(&clone, bag + 2, bag_remaining - 2, &search_score);
    scores[i] = move_score + PREFER_LONGER * search_score;
  }

  for (size_t i = 0; i < num_moves; ++i) {
    if (scores[i] > max) {
      max = scores[i];
      move = moves[i];
    }
    flex_bonus += scores[i];
  }
  if (num_moves > 0) {
    flex_bonus /= num_moves;
  }

  *score_out = 0.9 * max + 0.1 * flex_bonus;

  return move;
}
