#include "internal.h"

#include <stdlib.h>
#include <stdio.h>

void*
fatso_alloc(size_t size) {
  // Using fatso_calloc because it automatically zeroes memory.
  return fatso_calloc(1, size);
}

void*
fatso_calloc(size_t count, size_t size) {
  if (size && count) {
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
      perror("calloc");
      abort();
    }
    return ptr;
  }
  return NULL;
}

void*
fatso_reallocf(void* ptr, size_t size) {
  if (size) {
    ptr = reallocf(ptr, size);
    if (!ptr) {
      perror("reallocf");
      abort();
    }
    return ptr;
  } else {
    fatso_free(ptr);
    return NULL;
  }
}

void
fatso_free(void* ptr) {
  free(ptr);
}
