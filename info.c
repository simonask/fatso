#include "fatso.h"
#include "internal.h"

#include <stdio.h>

static void
display_info_for_package(const struct fatso_package* p) {
  if (p->name) {
    printf("project: %s\n", p->name);
  } else {
    printf("project: <NO NAME>\n");
  }

  if (p->author) {
    printf("authors: %s\n", p->author);
  }

  printf("version: %s\n", fatso_version_string(&p->version));

  if (p->source) {
    printf("source: %s\n", p->source->name);
  }

  if (p->toolchain) {
    printf("toolchain: %s\n", p->toolchain);
  }

  printf("dependencies:\n");
  // TODO: Use consolidated dependencies.
  for (size_t i = 0; i < p->base_environment.dependencies.size; ++i) {
    const struct fatso_dependency* dep = &p->base_environment.dependencies.data[i];
    printf("  %s", dep->name);
    if (dep->constraints.size) {
      printf(" (");
      for (size_t i = 0; i < dep->constraints.size; ++i) {
        const struct fatso_constraint* c = &dep->constraints.data[i];
        const char* vs = fatso_version_string(&c->version);
        printf("%s%s%s", fatso_version_requirement_to_string(c->version_requirement), vs ? " " : "", vs ? vs : "");
        if (i + 1 != dep->constraints.size) {
          printf(", ");
        }
      }
      printf(")");
    }
    printf("\n");
  }

  printf("defines:\n");
  // TODO: Use consolidated defines.
  for (size_t i = 0; i < p->base_environment.defines.size; ++i) {
    const struct fatso_define* def = &p->base_environment.defines.data[i];
    printf("  %s: %s\n", def->key, def->value);
  }
}

int
fatso_info(struct fatso* f, int argc, char* const* argv) {
  int r = 0;
  if (argc == 1) {
    r = fatso_load_project(f);
    if (r != 0)
      goto out;

    display_info_for_package(&f->project->package);
  } else {
    enum fatso_repository_result res;
    struct fatso_package* package;
    res = fatso_repository_find_package(f, argv[1], NULL, &package);
    switch (res) {
      case FATSO_PACKAGE_OK: {
        display_info_for_package(package);
        r = 0;
        break;
      }
      case FATSO_PACKAGE_UNKNOWN: {
        fprintf(stderr, "Unknown package: %s\n", argv[1]);
        r = 1;
        break;
      }
      case FATSO_PACKAGE_NO_MATCHING_VERSION: {
        fprintf(stderr, "Package found, but no versions found: %s\n", argv[1]);
        r = 1;
        break;
      }
    }
  }

out:
  return r;
}
