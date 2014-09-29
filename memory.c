#include "internal.h"

#include <stdlib.h> // allocators
#include <stdio.h>  // perror

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
    #if defined(__linux)
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
      free(ptr);
      perror("realloc");
      abort();
    }
    return new_ptr;
    #else
    ptr = reallocf(ptr, size);
    if (!ptr) {
      perror("reallocf");
      abort();
    }
    return ptr;
    #endif
  } else {
    fatso_free(ptr);
    return NULL;
  }
}

void
fatso_free(void* ptr) {
  free(ptr);
}
