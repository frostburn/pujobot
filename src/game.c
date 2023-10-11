#include "pujobot/util.h"
#include "pujobot/bitboard.h"
#include "pujobot/screen.h"
#include "pujobot/game.h"

void clear_simple_game(simple_game *g) {
  clear_simple_screen(&(g->screen));
  g->point_residue = 0;
  g->all_clear_bonus = 0;
  g->pending_garbage = 0;
  g->late_garbage = 0;
  g->late_time_remaining = 0;
  g->move_time = 16;
  g->color_selection[0] = 0;
  g->color_selection[1] = 1;
  g->color_selection[2] = 2;
  g->color_selection[3] = 3;
}

size_t get_simple_moves(simple_game *g, color_t *bag, move_t *moves_out) {
  puyos mask;
  store_mask(mask, g->screen.grid);
  bool symmetric = bag != NULL && bag[0] == bag[1];
  size_t num_moves = 0;
  for (move_t i = 0; i < SIZEOF(MOVES); ++i) {
    if (symmetric && 2*i >= SIZEOF(MOVES)) {
      return num_moves;
    }
    size_t x1 = MOVES[i][0];
    size_t x2 = MOVES[i][2];
    if (!puyo_at(mask, x1, GHOST_Y) || !puyo_at(mask, x2, GHOST_Y)) {
      moves_out[num_moves++] = i;
    }
  }
  if (g->late_garbage > 0 && g->late_time_remaining > 0) {
    moves_out[num_moves++] = PASS;
  }
  return num_moves;
}

void play_simple(simple_game *g, color_t *bag, move_t move_index) {
  if (move_index == PASS) {
    g->late_time_remaining = 0;
    return;
  }
  color_t color1 = bag[0];
  color_t color2 = bag[1];

  insert_puyo(&(g->screen), MOVES[move_index][0], MOVES[move_index][1], color1);
  insert_puyo(&(g->screen), MOVES[move_index][2], MOVES[move_index][3], color2);
  
  int released_garbage = g->pending_garbage;
  if (released_garbage > ONE_STONE) {
    released_garbage = ONE_STONE;
  }
  g->pending_garbage -= released_garbage;
  g->screen.buffered_garbage += released_garbage;
}

int resolve_simple(simple_game *g) {
  int chain_number;
  int score = tick_simple_screen(&(g->screen), &chain_number);

  g->late_time_remaining -= chain_number * 17 + g->move_time;
  if (g->late_time_remaining <= 0) {
    g->pending_garbage += g->late_garbage;
    g->late_garbage = 0;
  }

  if (chain_number && g->all_clear_bonus) {
    g->point_residue += SIMPLE_ALL_CLEAR_BONUS;
    g->all_clear_bonus = false;
  }
  g->point_residue += score;

  int generated_garbage = g->point_residue / TARGET_POINTS;
  g->point_residue -= generated_garbage * TARGET_POINTS;

  if (g->pending_garbage > generated_garbage) {
    g->pending_garbage -= generated_garbage;
    generated_garbage = 0;
  } else {
    generated_garbage -= g->pending_garbage;
    g->pending_garbage = 0;
  }
  if (g->late_garbage > generated_garbage) {
    g->late_garbage -= generated_garbage;
    generated_garbage = 0;
  } else {
    generated_garbage -= g->late_garbage;
    g->late_garbage = 0;
  }

  if (is_all_clear(&(g->screen))) {
    g->all_clear_bonus = true;
    score += SIMPLE_ALL_CLEAR_BONUS;
  }
  if (is_locked_out(&(g->screen))) {
    score += SIMPLE_GAME_OVER;
  }
  return score;
}

void print_simple_game(simple_game *g) {
  print_screen(&(g->screen));
  printf("Point residue = %d\n", g->point_residue);
  printf("All clear = %s\n", g->all_clear_bonus ? "true" : "false");
  printf("Pending garbage = %d\n", g->pending_garbage);
  printf("Late garbage = %d in %g\n", g->late_garbage, g->late_time_remaining);
  printf("Move time = %g\n", g->move_time);
  printf("Color selection = [%d, %d, %d, %d]\n", g->color_selection[0], g->color_selection[1], g->color_selection[2], g->color_selection[3]);
}
