#include "fatso.h"
#include <stdio.h>

int
fatso_install(struct fatso* f, int argc, char const* argv[]) {
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
  fprintf(stderr, "%s: NIY\n", __func__);
  return -1;
}
