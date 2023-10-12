#ifndef PUJOBOT_AI_H_GUARD
#define PUJOBOT_AI_H_GUARD

#define HEURISTIC_FAIL (-2000000)
#define PREFER_LONGER (1.1)

// RR, RG(2), RY(2), RB(2), GG, GY(2), GB(2), YY, YB(2), BB
#define NUM_EXTENSIONS (10)
static const double EXTENSION_WEIGHTS[] = {1, 2, 2, 2, 1, 2, 2, 1, 2, 1};
#define TOTAL_EXTENSION_WEIGHT (16)

/**
 * Determine if the game is effectively locked out.
 */
int effective_lockout(simple_game *g, color_t *bag, size_t bag_remaining);

/**
 * Heuristic score to discourage wasting of material.
 */
int material_count(simple_game *g);

/**
 * Heuristic score to discourage top-outs.
 */
int top_penalty(simple_game *g);

int max_droplet(simple_game *g);

int flex_droplet(simple_game *g);

double pass_penalty(simple_game *g);

size_t flex_droplet_strategy_1(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out);

size_t flex_droplet_strategy_2(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out);

size_t flex_droplet_strategy_3(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out);

color_t* extend_bag(simple_game *g, color_t *bag, size_t bag_remaining);

// Not a real flex. Just a max on flex2.
size_t flex_droplet_strategy_4(simple_game *g, color_t *bag, size_t bag_remaining, double *score_out);

#endif /* !PUJOBOT_AI_H_GUARD */
