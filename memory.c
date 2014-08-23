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
  void* ptr = calloc(count, size);
  if (size && ptr == NULL) {
    perror("calloc");
    abort();
  }
  return ptr;
}

void*
fatso_reallocf(void* ptr, size_t size) {
  if (size) {
    void* nptr = reallocf(ptr, size);
    if (!nptr) {
      perror("reallocf");
      abort();
    }
    return nptr;
  } else {
    fatso_free(ptr);
    return NULL;
  }
}

void
fatso_free(void* ptr) {
  free(ptr);
}
