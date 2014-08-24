#include "internal.h"

#include <string.h> // memset

void fatso_package_init(struct fatso_package* p) {
  memset(p, 0, sizeof(*p));
}

void
fatso_package_destroy(struct fatso_package* p) {
  fatso_free(p->name);
  fatso_version_destroy(&p->version);
  fatso_free(p->author);
  fatso_free(p->toolchain);
  fatso_environment_destroy(&p->base_environment);
  for (size_t i = 0; i < p->environments.size; ++i) {
    fatso_environment_destroy(&p->environments.data[i]);
  }
  fatso_free(p->environments.data);
  memset(p, 0, sizeof(*p));
}
