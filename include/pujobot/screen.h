#ifndef PUJOBOT_SCREEN_H_GUARD
#define PUJOBOT_SCREEN_H_GUARD

#include <stdio.h>
#include "pujobot/bitboard.h"

// Scoring
#define MAX_CLEAR_BONUS (999)
static const int COLOR_BONUS[] = {0, 0, 3, 6, 12, 24};
static const int CHAIN_POWERS[] = {
  0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,
  480, 512, 544, 576, 608, 640, 672,
};

#define AIR (-1)
#define RED (0)
#define GREEN (1)
#define YELLOW (2)
#define BLUE (3)
#define PURPLE (4)
#define GARBAGE (5)

// Not size_t because AIR can be useful at times.
typedef int color_t;

typedef struct simple_screen {
  puyos grid[NUM_PUYO_TYPES];
  int buffered_garbage;
} simple_screen;

void clear_simple_screen(simple_screen *s);

void randomize_screen(simple_screen *s);

const char* ansi_color(color_t color, bool dark);

void print_screen(simple_screen *s);

int tick_simple_screen(simple_screen *s, int *chain_number_out);

bool is_all_clear(simple_screen *s);

bool is_locked_out(simple_screen *s);

bool insert_puyo(simple_screen *s, size_t x, size_t y, color_t color);

void simple_screen_fprintf(FILE *f, simple_screen *s);

#endif /* !PUJOBOT_SCREEN_H_GUARD */
