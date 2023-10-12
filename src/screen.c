#include "jkiss/jkiss.h"

#include "pujobot/util.h"
#include "pujobot/bitboard.h"
#include "pujobot/screen.h"

void clear_simple_screen(simple_screen *s) {
  for (int i = 0; i < NUM_PUYO_TYPES; ++i) {
    clear(s->grid[i]);
  }
  s->buffered_garbage = 0;
}

void randomize_screen(simple_screen *s) {
  for (int i = 0; i < NUM_PUYO_TYPES; ++i) {
    clear(s->grid[i]);
  }
  for (int x = 0; x < WIDTH; ++x) {
    for (int y = 0; y < HEIGHT; ++y) {
      if (rand() < RAND_MAX / 2) {
        s->grid[rand() % NUM_PUYO_TYPES][x] |= 1 << y;
      }
    }
  }
}

const char* ansi_color(color_t color, bool dark) {
  if (dark) {
    if (color == 0) {
      return "\x1b[31m";
    } else if (color == 1) {
      return "\x1b[32m";
    } else if (color == 2) {
      return "\x1b[33m";
    } else if (color == 3) {
      return "\x1b[34m";
    } else if (color == 4) {
      return "\x1b[35m";
    } else if (color == 5) {
      return "\x1b[36m";
    }
  }
  if (color == 0) {
    return "\x1b[31;1m";
  } else if (color == 1) {
    return "\x1b[32;1m";
  } else if (color == 2) {
    return "\x1b[33;1m";
  } else if (color == 3) {
    return "\x1b[34;1m";
  } else if (color == 4) {
    return "\x1b[35;1m";
  } else if (color == 5) {
    return "\x1b[36;1m";
  }
  return "\x1b[0m";
}

void print_screen(simple_screen *s) {
  printf("╔════════════╗\n");
  for (int y = 0; y < HEIGHT; ++y) {
    printf("║");
    for (int x = 0; x < WIDTH; ++x) {
      if (x > 0) {
        printf(" ");
      }
      bool any = false;
      bool many = false;
      for (color_t i = 0 ; i < NUM_PUYO_TYPES; ++i) {
        if (puyo_at(s->grid[i], x, y)) {
          if (any) {
            many = true;
          } else {
            printf("%s", ansi_color(i, y < HEIGHT - VISIBLE_HEIGHT));
            if (i == GARBAGE) {
              printf("◎");
            } else {
              printf("●");
            }
          }
          any = true;
        }
      }
      if (many) {
        printf("\b?");
      }
      if (!any) {
        printf(" ");
      }
    }
    printf("%s", ansi_color(AIR, false));
    printf(" ║\n");
  }
  printf("╚════════════╝\n");
  printf("Buffered garbage = %d\n", s->buffered_garbage);
}

void shuffle_columns(puyos p, jkiss32 *jkiss) {
  unsigned int entropy = jkiss32_pure_step(jkiss);
  for (int i = NUM_SLICES - 1; i > 0; i--) {
    int j = entropy % (i + 1);
    entropy /= (i + 1);
    slice_t temp = p[i];
    p[i] = p[j];
    p[j] = temp;
  }
}

int tick_simple_screen(simple_screen *s, int *chain_number_out) {
  *chain_number_out = 0;
  int score = 0;

  // Commit garbage buffer.
  while (s->buffered_garbage) {
    // Create (up to) one line of garbage.
    if (s->buffered_garbage >= WIDTH) {
      s->grid[GARBAGE][0] |= 1;
      s->grid[GARBAGE][1] |= 1;
      s->grid[GARBAGE][2] |= 1;
      s->grid[GARBAGE][3] |= 1;
      s->grid[GARBAGE][4] |= 1;
      s->grid[GARBAGE][5] |= 1;

      s->buffered_garbage -= WIDTH;
    } else if (s->buffered_garbage) {
      puyos line;
      for (int i = 0; i < NUM_SLICES; i++) {
        line[i] = (i < s->buffered_garbage);
      }
      shuffle_columns(line, &s->jkiss);
      merge(s->grid[GARBAGE], line);
      s->buffered_garbage = 0;
    }
    fall_one(s->grid);
  }

  bool active = true;
  while (active) {
    active = resolve_gravity(s->grid);

    for (color_t i = 0; i < NUM_PUYO_TYPES; ++i) {
      vanish_top(s->grid[i]);
    }

    int num_colors = 0;
    bool did_clear = false;
    int total_num_cleared = 0;
    int total_group_bonus = 0;
    puyos total_cleared;

    clear(total_cleared);

    puyos sparks;
    for (color_t i = 0; i < NUM_PUYO_COLORS; ++i) {
      int group_bonus;
      int num_cleared = spark_groups(s->grid[i], sparks, &group_bonus);
      if (num_cleared) {
        total_num_cleared += num_cleared;
        total_group_bonus += group_bonus;
        apply_xor(s->grid[i], sparks);
        merge(total_cleared, sparks);
        num_colors++;
        did_clear = true;
      }
    }

    spark_garbage(s->grid[GARBAGE], total_cleared, sparks);
    apply_xor(s->grid[GARBAGE], sparks);

    int color_bonus = COLOR_BONUS[num_colors];
    int chain_power = CHAIN_POWERS[*chain_number_out];
    int clear_bonus = chain_power + color_bonus + total_group_bonus;
    if (clear_bonus < 1) {
      clear_bonus = 1;
    } else if (clear_bonus > MAX_CLEAR_BONUS) {
      clear_bonus = MAX_CLEAR_BONUS;
    }
    score += 10 * total_num_cleared * clear_bonus;

    if (did_clear) {
      active = true;
      *chain_number_out = (*chain_number_out) + 1;
    }
  }
  return score;
}

bool is_all_clear(simple_screen *s) {
  for (int i = 0; i < NUM_PUYO_TYPES; ++i) {
    if (is_nonempty(s->grid[i])) {
      return false;
    }
  }
  return true;
}

bool is_locked_out(simple_screen *s) {
  puyos mask;
  store_mask(mask, s->grid);
  return topped_up(mask);
}

bool insert_puyo(simple_screen *s, int x, int y, color_t color) {
  puyos mask;
  store_mask(mask, s->grid);
  if (puyo_at(mask, x, y)) {
    return true;
  }
  add_puyo(s->grid[color], x, y);
  return false;
}

void simple_screen_fprintf(FILE *f, simple_screen *s) {
  fprintf(f, "(simple_screen){");
  for (int j = 0; j < NUM_PUYO_TYPES; ++j) {
    for (int i = 0; i < NUM_SLICES; ++i) {
      fprintf(f, "0x%x, ", s->grid[j][i]);
    }
  }
  fprintf(f, "%d};\n", s->buffered_garbage);
}
