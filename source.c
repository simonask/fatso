#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup
#include <yaml.h>

int
fatso_source_parse(
  struct fatso_source* source,
  struct yaml_document_s* doc,
  struct yaml_node_s* node,
  char** out_error_message
) {
  if (node->type == YAML_SCALAR_NODE) {
    char* url = fatso_yaml_scalar_strdup(node);
    fatso_tarball_source_init(source, url);
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

int
fatso_source_fetch(struct fatso* f, struct fatso_package* p, struct fatso_source* source) {
  return source->vtbl->fetch(f, p, source);
}

int
fatso_source_unpack(struct fatso* f, struct fatso_package* p, struct fatso_source* source) {
  return source->vtbl->unpack(f, p, source);
}


