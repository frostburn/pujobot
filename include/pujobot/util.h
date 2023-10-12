#ifndef PUJOBOT_UTIL_H_GUARD
#define PUJOBOT_UTIL_H_GUARD

#include <stdlib.h>

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

typedef signed char move_t;

void shuffle(move_t *array, size_t array_size);

#endif /* !PUJOBOT_UTIL_H_GUARD */
