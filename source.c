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
  } else if (node->type == YAML_MAPPING_NODE) {
    yaml_node_t* git_node = fatso_yaml_mapping_lookup(doc, node, "git");
    if (git_node != NULL && git_node->type == YAML_SCALAR_NODE) {
      yaml_node_t* ref_node = fatso_yaml_mapping_lookup(doc, node, "ref");
      char* ref;
      if (ref_node != NULL && ref_node->type == YAML_SCALAR_NODE) {
        ref = fatso_yaml_scalar_strdup(ref_node);
      } else {
        ref = strdup("master");
      }
      char* url = fatso_yaml_scalar_strdup(git_node);
      fatso_git_source_init(source, url, ref);
      fatso_free(url);
      fatso_free(ref);
      return 0;
    } else {
      *out_error_message = strdup("Invalid source (expected 'git' to indicate source URL).");
      return 1;
    }
  } else {
    *out_error_message = strdup("Invalid source (expected URL, or source definition, or list of source definitions).");
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


