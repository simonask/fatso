#include "internal.h"

#include <string.h>
#include <stdlib.h> // free

void
fatso_environment_init(struct fatso_environment* e) {
  memset(e, 0, sizeof(*e));
}

void
fatso_environment_destroy(struct fatso_environment* e) {
  fatso_free(e->name);
  for (size_t i = 0; i < e->dependencies.size; ++i) {
    fatso_dependency_destroy(&e->dependencies.data[i]);
  }
  fatso_free(e->dependencies.data);
  for (size_t i = 0; i < e->defines.size; ++i) {
    fatso_define_destroy(&e->defines.data[i]);
  }
  fatso_free(e->defines.data);
  memset(e, 0, sizeof(*e));
}
