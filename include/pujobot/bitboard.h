#ifndef PUJOBOT_BITBOARD_H_GUARD
#define PUJOBOT_BITBOARD_H_GUARD

#include <stdbool.h>
#include <stdio.h>

#define WIDTH (6)
#define NUM_SLICES (6)
#define HEIGHT (16)
#define VISIBLE_HEIGHT (12)
#define NUM_PUYO_COLORS (5)
#define NUM_PUYO_TYPES (6)
#define CLEAR_THRESHOLD (4)

#define GHOST_Y (3)
#define BOTTOM_Y (15)

// Bitboard patterns
#define BOTTOM (32768)
#define VISIBLE (65520)
#define SEMI_VISIBLE (65528)
#define TOPPING_LINE (16)

typedef unsigned short int slice_t;
typedef unsigned short int puyos[NUM_SLICES];

static const int GROUP_BONUS[] = {0, 2, 3, 4, 5, 6, 7, 10};

void print_puyos(puyos p);

int puyo_count(puyos p);

void clear(puyos p);

// Modifies source in place.
void flood(register puyos source, register puyos target);

void store_mask(puyos result, puyos *grid);

void store_colored_mask(puyos result, puyos *grid);

bool resolve_gravity(puyos *grid);

void store_clone(puyos clone, puyos p);

bool is_empty(puyos p);

bool is_nonempty(puyos p);

void apply_xor(puyos out, puyos in);

void merge(puyos out, puyos in);

void apply_mask(puyos out, puyos in);

int get_group_bonus(int group_size);

int spark_groups(puyos p, puyos sparks_out, int *group_bonus_out);

bool puyo_at(puyos p, size_t x, size_t y);

void add_puyo(puyos p, size_t x, size_t y);

void vanish_top(puyos p);

void keep_visible(puyos p);

void spark_garbage(puyos garbage, puyos cleared, puyos sparks_out);

bool topped_up(puyos p);

void invert(puyos p);

void trim_unsupported(puyos p);

void fall_one(puyos *grid);

void chip(puyos in, puyos out);

void puyos_fprintf(FILE *f, puyos p);

bool is_contiguous2(puyos p);

#endif /* !PUJOBOT_BITBOARD_H_GUARD */
