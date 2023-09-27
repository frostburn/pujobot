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

void print_puyos(puyos p) {
  printf("┌─────────────┐\n");
  for (int y = 0; y < HEIGHT; ++y) {
    printf("│");
    for (int x = 0; x < WIDTH; ++x) {
      if (p[x] & (1 << y)) {
        printf(" @");
      } else {
        printf(" .");
      }
    }
    printf(" │\n");
  }
  printf("└─────────────┘\n");
}

int puyo_count(puyos p) {
  return (
    __builtin_popcount(p[0]) +
    __builtin_popcount(p[1]) +
    __builtin_popcount(p[2]) +
    __builtin_popcount(p[3]) +
    __builtin_popcount(p[4]) +
    __builtin_popcount(p[5])
  );
}

void clear(puyos p) {
  p[0] = 0;
  p[1] = 0;
  p[2] = 0;
  p[3] = 0;
  p[4] = 0;
  p[5] = 0;
}

// Modifies source in place.
void flood(register puyos source, register puyos target) {
  source[0] &= target[0];
  source[1] &= target[1];
  source[2] &= target[2];
  source[3] &= target[3];
  source[4] &= target[4];
  source[5] &= target[5];

  if (!(source[0] | source[1] | source[2] | source[3] | source[4] | source[5])) {
    return;
  }

  register puyos temp;
  do {
    temp[0] = source[0];
    temp[1] = source[1];
    temp[2] = source[2];
    temp[3] = source[3];
    temp[4] = source[4];
    temp[5] = source[5];

    source[0] |= (source[1] | (source[0] >> 1) | (source[0] << 1))             & target[0];
    source[1] |= (source[0] | (source[1] >> 1) | (source[1] << 1) | source[2]) & target[1];
    source[2] |= (source[1] | (source[2] >> 1) | (source[2] << 1) | source[3]) & target[2];
    source[3] |= (source[2] | (source[3] >> 1) | (source[3] << 1) | source[4]) & target[3];
    source[4] |= (source[3] | (source[4] >> 1) | (source[4] << 1) | source[5]) & target[4];
    source[5] |= (source[4] | (source[5] >> 1) | (source[5] << 1))             & target[5];
  } while (
    temp[0] != source[0] ||
    temp[1] != source[1] ||
    temp[2] != source[2] ||
    temp[3] != source[3] ||
    temp[4] != source[4] ||
    temp[5] != source[5]
  );
}

void store_mask(puyos result, puyos *grid) {
  result[0] = grid[0][0];
  result[1] = grid[0][1];
  result[2] = grid[0][2];
  result[3] = grid[0][3];
  result[4] = grid[0][4];
  result[5] = grid[0][5];

  for (int j = 0; j < NUM_SLICES; ++j) {
    for (int i = 1; i < NUM_PUYO_TYPES; ++i) {
      result[j] |= grid[i][j];
    }
  }
}

bool resolve_gravity(puyos *grid) {
  slice_t unsupported;
  slice_t falling;
  puyos all;
  store_mask(all, grid);

  bool did_something = false;
  for (int j = 0; j < NUM_SLICES; ++j) {
    bool did_fall = true;
    while (did_fall) {
      did_fall = false;

      unsupported = all[j] & ~((all[j] >> 1) | BOTTOM);
      did_fall = did_fall || unsupported;
      all[j] ^= unsupported ^ (unsupported << 1);
      for (int i = 0; i < NUM_PUYO_TYPES; ++i) {
        falling = grid[i][j] & unsupported;
        grid[i][j] ^= falling ^ (falling << 1);
      }
    }

    did_something = did_something || did_fall;
  }
  return did_something;
}

void store_clone(puyos clone, puyos p) {
  clone[0] = p[0];
  clone[1] = p[1];
  clone[2] = p[2];
  clone[3] = p[3];
  clone[4] = p[4];
  clone[5] = p[5];
}

bool is_empty(puyos p) {
  return !(p[0] | p[1] | p[2] | p[3] | p[4] | p[5]);
}

bool is_nonempty(puyos p) {
  return p[0] | p[1] | p[2] | p[3] | p[4] | p[5];
}

void apply_xor(puyos out, puyos in) {
  out[0] ^= in[0];
  out[1] ^= in[1];
  out[2] ^= in[2];
  out[3] ^= in[3];
  out[4] ^= in[4];
  out[5] ^= in[5];
}

void merge(puyos out, puyos in) {
  out[0] |= in[0];
  out[1] |= in[1];
  out[2] |= in[2];
  out[3] |= in[3];
  out[4] |= in[4];
  out[5] |= in[5];
}

void apply_mask(puyos out, puyos in) {
  out[0] &= in[0];
  out[1] &= in[1];
  out[2] &= in[2];
  out[3] &= in[3];
  out[4] &= in[4];
  out[5] &= in[5];
}

int get_group_bonus(int group_size) {
  group_size -= CLEAR_THRESHOLD;
  if (group_size >= SIZEOF(GROUP_BONUS)) {
    group_size = SIZEOF(GROUP_BONUS) - 1;
  }
  return GROUP_BONUS[group_size];
}

int spark_groups(puyos p, puyos sparks_out, int *group_bonus_out) {
  int num_cleared = 0;
  *group_bonus_out = 0;
  puyos group;
  puyos temp;

  clear(group);
  clear(sparks_out);
  temp[0] = p[0] & VISIBLE;
  temp[1] = p[1] & VISIBLE;
  temp[2] = p[2] & VISIBLE;
  temp[3] = p[3] & VISIBLE;
  temp[4] = p[4] & VISIBLE;
  temp[5] = p[5] & VISIBLE;

  // Clear from the bottom up hoping for an early exit.
  for (int y = BOTTOM_Y - 1; y > GHOST_Y; y -= 2) {
    for (int x = 0; x < NUM_SLICES; ++x) {
      // This may have residue in other slices but those groups have already been cleared.
      group[x] = 3 << y;
      flood(group, temp);
      apply_xor(temp, group);
      int group_size = puyo_count(group);
      if (group_size >= CLEAR_THRESHOLD) {
        merge(sparks_out, group);
        *group_bonus_out += get_group_bonus(group_size);
        num_cleared += group_size;
      }
      if (is_empty(temp)) {
        return num_cleared;
      }
    }
  }
  return num_cleared;
}

bool puyo_at(puyos p, size_t x, size_t y) {
  return p[x] & (1 << y);
}

void add_puyo(puyos p, size_t x, size_t y) {
  p[x] |= (1 << y);
}

void vanish_top(puyos p) {
  p[0] &= SEMI_VISIBLE;
  p[1] &= SEMI_VISIBLE;
  p[2] &= SEMI_VISIBLE;
  p[3] &= SEMI_VISIBLE;
  p[4] &= SEMI_VISIBLE;
  p[5] &= SEMI_VISIBLE;
}

void keep_visible(puyos p) {
  p[0] &= VISIBLE;
  p[1] &= VISIBLE;
  p[2] &= VISIBLE;
  p[3] &= VISIBLE;
  p[4] &= VISIBLE;
  p[5] &= VISIBLE;
}

void spark_garbage(puyos garbage, puyos cleared, puyos sparks_out) {
  store_clone(sparks_out, garbage);

  sparks_out[0] &= (cleared[0] << 1) | (cleared[0] >> 1) | cleared[1];
  sparks_out[1] &= (cleared[1] << 1) | (cleared[1] >> 1) | cleared[0] | cleared[2];
  sparks_out[2] &= (cleared[2] << 1) | (cleared[2] >> 1) | cleared[1] | cleared[3];
  sparks_out[3] &= (cleared[3] << 1) | (cleared[3] >> 1) | cleared[2] | cleared[4];
  sparks_out[4] &= (cleared[4] << 1) | (cleared[4] >> 1) | cleared[3] | cleared[5];
  sparks_out[5] &= (cleared[5] << 1) | (cleared[5] >> 1) | cleared[4];
}

bool topped_up(puyos p) {
  return p[0] & p[1] & p[2] & p[3] & p[4] & p[5] & TOPPING_LINE;
}

void invert(puyos p) {
  p[0] = ~p[0];
  p[1] = ~p[1];
  p[2] = ~p[2];
  p[3] = ~p[3];
  p[4] = ~p[4];
  p[5] = ~p[5];
}
void trim_unsupported(puyos p) {
  for (int i = 0; i < HEIGHT - 1; ++i) {
    p[0] &= (p[0] >> 1) | BOTTOM;
    p[1] &= (p[1] >> 1) | BOTTOM;
    p[2] &= (p[2] >> 1) | BOTTOM;
    p[3] &= (p[3] >> 1) | BOTTOM;
    p[4] &= (p[4] >> 1) | BOTTOM;
    p[5] &= (p[5] >> 1) | BOTTOM;
  }
}

void fall_one(puyos *grid) {
  puyos mask;
  store_mask(mask, grid);

  puyos unsupported;
  store_clone(unsupported, mask);
  trim_unsupported(unsupported);
  invert(unsupported);
  apply_mask(unsupported, mask);

  for (int i = 0; i < NUM_PUYO_TYPES; ++i) {
    for (int x = 0; x < NUM_SLICES; ++x) {
      slice_t falling = grid[i][x] & unsupported[x];
      grid[i][x] ^= falling ^ (falling << 1);
    }
  }
}

void split(puyos in, puyos out) {
  clear(out);
  for (int y = HEIGHT - 1; y >= 0; y--) {
    slice_t s = 1 << y;
    for (int x = 0; x < NUM_SLICES; ++x) {
      if (in[x] & s) {
        in[x] ^= s;
        out[x] = s;
        return;
      }
    }
  }
}

void puyos_fprintf(FILE *f, puyos p) {
  fprintf(f, "{0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x};\n", p[0], p[1], p[2], p[3], p[4], p[5]);
}
