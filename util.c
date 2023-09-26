#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

void shuffle6(size_t *array) {
  int entropy = rand();
  for (int i = 5; i > 0; i--) {
    int j = entropy % (i + 1);
    entropy /= (i + 1);
    size_t temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}

void shuffle(size_t *array, size_t array_size) {
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
    size_t temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}
