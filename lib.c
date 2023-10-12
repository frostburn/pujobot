#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "api/pujo.h"

// Probably not how you do it but whatever works...
#include "src/util.c"
#include "src/bitboard.c"
#include "src/screen.c"
#include "src/game.c"
#include "src/ai.c"

void initialize() {
  srand(time(0));
}

void version(char *out) {
  sprintf(out, "%s.%s.%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}
