#include "fatso.h"
#include "internal.h"
#include <stdio.h>

int
fatso_install(struct fatso* f, int argc, char* const* argv) {
  int r = 0;

  r = fatso_load_project(f);
  if (r != 0) goto out;

  r = fatso_load_or_generate_dependency_graph(f);
  if (r != 0) goto out;

  r = fatso_install_dependencies(f);
  if (r != 0) goto out;

out:
  return r;
}

int
fatso_install_dependencies(struct fatso* f) {
  for (size_t i = 0; i < f->project->install_order.size; ++i) {
    struct fatso_package* p = f->project->install_order.data[i];
    printf("INSTALL: %s (%s)\n", p->name, fatso_version_string(&p->version));
  }
  return -1;
}
