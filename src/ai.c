#include "pujobot/util.h"
#include "pujobot/bitboard.h"
#include "pujobot/screen.h"
#include "pujobot/game.h"
#include "pujobot/ai.h"

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

int max_droplet(simple_game *g) {
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

int flex_droplet(simple_game *g) {
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

double pass_penalty(simple_game *g) {
  if (g->late_time_remaining <= 0) {
    fprintf(stderr, "Passing shouldn't be considered here.\n");
    exit(EXIT_FAILURE);
  }
  return -10 * g->late_time_remaining;
}

size_t flex_droplet_strategy_1(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
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
    double move_score = 0;
    if (moves[i] == PASS) {
      move_score = pass_penalty(g);
    } else {
      move_score = resolve_simple(&clone);
    }
    double score = (
      move_score +
      PREFER_LONGER * flex_droplet(&clone) +
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

size_t flex_droplet_strategy_2(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
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
    double move_score = 0;
    if (moves[i] == PASS) {
      move_score = pass_penalty(g);
    } else {
      move_score = resolve_simple(&clone);
    }
    double search_score;
    flex_droplet_strategy_1(&clone, bag + 2, bag_remaining - 2, &search_score);
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

size_t flex_droplet_strategy_3(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
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
    double move_score = 0;
    if (moves[i] == PASS) {
      move_score = pass_penalty(g);
    } else {
      move_score = resolve_simple(&clone);
    }
    double search_score;
    flex_droplet_strategy_2(&clone, bag + 2, bag_remaining - 2, &search_score);
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

color_t* extend_bag(simple_game *g, color_t *bag, size_t bag_remaining) {
  size_t segment_size = bag_remaining + 2;
  color_t* result = malloc(sizeof(color_t) * segment_size * NUM_EXTENSIONS);
  color_t* current = result;
  for (int i = 0; i < COLOR_SELECTION_SIZE; ++i) {
    for (int j = i; j < COLOR_SELECTION_SIZE; ++j) {
      for (int k = 0; k < bag_remaining; ++k) {
        current[k] = bag[k];
      }
      current[bag_remaining] = g->color_selection[i];
      current[bag_remaining+1] = g->color_selection[j];
      current += segment_size;
    }
  }
  return result;
}

// Not a real flex. Just a max on flex2.
size_t flex_droplet_strategy_4(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out) {
  if (bag_remaining < 6) {
    fprintf(stderr, "Flex 4 needs at least a bag of 6.\n");
    exit(EXIT_FAILURE);
  }

  move_t first_moves[MAX_NUM_MOVES];
  size_t num_first_moves = get_simple_moves(g, bag, first_moves);
  // Shuffle to break ties
  shuffle(first_moves, num_first_moves);

  simple_game *clones = NULL;
  size_t num_clones = 0;
  move_t *parent_moves = NULL;
  double *scores = NULL;

  for (size_t i = 0; i < num_first_moves; ++i) {
    simple_game clone = *g;
    play_simple(&clone, bag, first_moves[i]);
    double first_move_score = resolve_simple(&clone);
    move_t second_moves[MAX_NUM_MOVES];
    size_t num_second_moves = get_simple_moves(&clone, bag + 2, second_moves);
    // No need to shuffle these.
    clones = realloc(clones, sizeof(simple_game) * (num_clones + num_second_moves));
    parent_moves = realloc(parent_moves, sizeof(move_t) * (num_clones + num_second_moves));
    scores = realloc(scores, sizeof(double) * (num_clones + num_second_moves));
    for (size_t j = 0; j < num_second_moves; ++j) {
      parent_moves[num_clones] = first_moves[i];
      clones[num_clones] = clone;
      play_simple(clones + num_clones, bag + 2, second_moves[j]);
      scores[num_clones] = first_move_score + resolve_simple(clones + num_clones);
      num_clones++;
    }
  }

  color_t *big_bag = extend_bag(g, bag + 4, bag_remaining - 4);
  size_t segment_size = bag_remaining - 2;

  #pragma omp parallel for
  for (size_t i = 0; i < num_clones; ++i) {
    double weighted_score = 0;
    for (size_t j = 0; j < NUM_EXTENSIONS; ++j) {
      double search_score;
      flex_droplet_strategy_2(clones + i, big_bag + segment_size * j, segment_size, &search_score);
      weighted_score += search_score * EXTENSION_WEIGHTS[j];
    }
    scores[i] += weighted_score / TOTAL_EXTENSION_WEIGHT;
  }

  free(clones);
  free(big_bag);

  double max = HEURISTIC_FAIL;
  size_t move = num_first_moves ? first_moves[0] : 0;
  for (size_t i = 0; i < num_clones; ++i) {
    if (scores[i] > max) {
      max = scores[i];
      move = parent_moves[i];
    }
  }

  free(parent_moves);
  free(scores);

  *score_out = max;

  return move;
}
