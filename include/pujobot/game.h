#ifndef PUJOBOT_GAME_H_GUARD
#define PUJOBOT_GAME_H_GUARD

#define TARGET_POINTS (70)
#define ONE_STONE (30)
#define COLOR_SELECTION_SIZE (4)
// Value all-clears based on the amount of garbage they send.
#define SIMPLE_ALL_CLEAR_BONUS (2100)
// Not even a 19-chain can compensate a Game Over.
#define SIMPLE_GAME_OVER (-1000000)
#define PASS (-1)
#define MAX_NUM_MOVES (23)

typedef signed char move_t;

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
  int point_residue;
  bool all_clear_bonus;
  int pending_garbage;
  int late_garbage;
  float late_time_remaining;
  float move_time;
  color_t color_selection[COLOR_SELECTION_SIZE];
} simple_game;

void clear_simple_game(simple_game *g);

size_t get_simple_moves(simple_game *g, color_t *bag, move_t *moves_out);

void play_simple(simple_game *g, color_t *bag, move_t move_index);

int resolve_simple(simple_game *g);

void print_simple_game(simple_game *g);

#endif /* !PUJOBOT_GAME_H_GUARD */
