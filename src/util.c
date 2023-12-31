#include "pujobot/util.h"

void shuffle(move_t *array, size_t array_size) {
  int entropy = 0;
  int juice = RAND_MAX;
  for (int i = array_size - 1; i > 0; i--) {
    if (juice >= RAND_MAX) {
      entropy = rand();
      juice = 1;
    }
    int j = entropy % (i + 1);
    entropy /= (i + 1);
    juice *= i + 1;
    move_t temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}
