#include "internal.h"

#include <string.h> // strdup
#include <yaml.h>

struct fatso_source_vtbl {
  const char* type;
  int(*fetch)(struct fatso_package*, struct fatso_source*);
  int(*build)(struct fatso_package*, struct fatso_source*);
  int(*install)(struct fatso_package*, struct fatso_source*);
  void(*free)(void* thunk);
};

int
fatso_tarball_fetch(struct fatso_package* package, struct fatso_source* source) {
  return 1;
}

int
fatso_tarball_build(struct fatso_package* package, struct fatso_source* source) {
  return 1;
}

int
fatso_tarball_install(struct fatso_package* package, struct fatso_source* source) {
  return 1;
}

static const struct fatso_source_vtbl tarball_source_vtbl = {
  .type = "tarball",
  .fetch = fatso_tarball_fetch,
  .build = fatso_tarball_build,
  .install = fatso_tarball_install,
  .free = fatso_free,
};

void
fatso_tarball_init(struct fatso_source* source, const char* url) {
  source->name = strdup(url);
  source->vtbl = &tarball_source_vtbl;
  source->thunk = NULL;
}

int
fatso_source_parse(
  struct fatso_source* source,
  struct yaml_document_s* doc,
  struct yaml_node_s* node,
  char** out_error_message
) {
  if (node->type == YAML_SCALAR_NODE) {
    char* url = fatso_yaml_scalar_strdup(node);
    fatso_tarball_init(source, url);
    fatso_free(url);
    return 0;
  } else {
    *out_error_message = strdup("Invalid source (expected URL).");
    return 1;
  }
}

void
fatso_source_destroy(struct fatso_source* source) {
  source->vtbl->free(source->thunk);
  fatso_free(source->name);
}


