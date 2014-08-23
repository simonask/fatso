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
  for (size_t i = 0; i < e->num_dependencies; ++i) {
    fatso_dependency_destroy(&e->dependencies[i]);
  }
  fatso_free(e->dependencies);
  for (size_t i = 0; i < e->num_defines; ++i) {
    fatso_define_destroy(&e->defines[i]);
  }
  fatso_free(e->defines);
  memset(e, 0, sizeof(*e));
}
