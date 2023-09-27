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
  size_t garbage_slots[WIDTH];
  size_t slot_index;
  int buffered_garbage;
} simple_screen;

typedef struct screen {
  simple_screen base;
  int chain_number;
  puyos jiggles;
  puyos sparks;
  // jkiss64 jkiss;  // Fair distribution of garbage requires deterministic randomness.
  bool doJiggles;
} screen;

void clear_simple_screen(simple_screen *s) {
  for (int i = 0; i < NUM_PUYO_TYPES; ++i) {
    clear(s->grid[i]);
  }
  s->buffered_garbage = 0;
  for (int i = 0; i < WIDTH; ++i) {
    s->garbage_slots[i] = i;
  }
  s->slot_index = WIDTH;
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

static const char* ansi_color(color_t color, bool dark) {
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
  printf("Slots = [%zu, %zu, %zu, %zu, %zu, %zu] @ %zu\n", s->garbage_slots[0], s->garbage_slots[1], s->garbage_slots[2], s->garbage_slots[3], s->garbage_slots[4], s->garbage_slots[5], s->slot_index);
  printf("Buffered garbage = %d\n", s->buffered_garbage);
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
      clear(line);
      while (s->buffered_garbage) {
        if (s->slot_index >= WIDTH) {
          for (int i = 0; i < WIDTH; ++i) {
            s->garbage_slots[i] = i;
          }
          shuffle6(s->garbage_slots);
          s->slot_index = 0;
        }
        line[s->garbage_slots[s->slot_index++]] = 1;
        s->buffered_garbage--;
      }
      merge(s->grid[GARBAGE], line);
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
  for (size_t i = 0; i < NUM_PUYO_TYPES; ++i) {
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

bool insert_puyo(simple_screen *s, size_t x, size_t y, color_t color) {
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
  for (int i = 0; i < WIDTH; ++i) {
    fprintf(f, "%zu, ", s->garbage_slots[i]);
  }
  fprintf(f, "%zu, %d};\n", s->slot_index, s->buffered_garbage);
}
