#include "internal.h"

#include <string.h> // memset
#include <yaml.h>

void
fatso_configuration_init(struct fatso_configuration* e) {
  memset(e, 0, sizeof(*e));
}

void
fatso_configuration_destroy(struct fatso_configuration* e) {
  fatso_free(e->name);
  for (size_t i = 0; i < e->dependencies.size; ++i) {
    fatso_dependency_destroy(&e->dependencies.data[i]);
  }
  fatso_free(e->dependencies.data);
  for (size_t i = 0; i < e->defines.size; ++i) {
    fatso_kv_pair_destroy(&e->defines.data[i]);
  }
  fatso_free(e->defines.data);
  memset(e, 0, sizeof(*e));
}

int
fatso_configuration_parse(struct fatso_configuration* e, struct yaml_document_s* doc, struct yaml_node_s* node, char** out_error_message) {
  int r = 0;

  yaml_node_t* dependencies = fatso_yaml_mapping_lookup(doc, node, "dependencies");
  if (dependencies && dependencies->type == YAML_SEQUENCE_NODE) {
    size_t len = fatso_yaml_sequence_length(dependencies);
    if (len != 0) {
      e->dependencies.size = len;
      e->dependencies.data = fatso_calloc(len, sizeof(struct fatso_dependency));
      for (size_t i = 0; i < len; ++i) {
        r = fatso_dependency_parse(&e->dependencies.data[i], doc, fatso_yaml_sequence_lookup(doc, dependencies, i), out_error_message);
        if (r != 0)
          goto out;
      }
    }
  }

  yaml_node_t* defines = fatso_yaml_mapping_lookup(doc, node, "defines");
  if (defines && defines->type == YAML_MAPPING_NODE) {
    size_t len = fatso_yaml_mapping_length(defines);
    if (len != 0) {
      e->defines.size = len;
      e->defines.data = fatso_calloc(len, sizeof(struct fatso_kv_pair));
      for (size_t i = 0; i < len; ++i) {
        yaml_node_pair_t* pair = &defines->data.mapping.pairs.start[i];
        yaml_node_t* key = yaml_document_get_node(doc, pair->key);
        yaml_node_t* value = yaml_document_get_node(doc, pair->value);
        e->defines.data[i].key = fatso_yaml_scalar_strdup(key);
        e->defines.data[i].value = fatso_yaml_scalar_strdup(value);
      }
    }
  }

out:
  return r;
}

static void
merge_configurations(struct fatso_configuration* into, const struct fatso_configuration* other) {
  for (size_t i = 0; i < other->dependencies.size; ++i) {
    const struct fatso_dependency* dep = &other->dependencies.data[i];
    struct fatso_dependency d;
    fatso_dependency_init(&d, dep->name, dep->constraints.data, dep->constraints.size);
    fatso_push_back_v(&into->dependencies, &d);
  }

  fatso_dictionary_merge(&into->defines, &other->defines);
  fatso_dictionary_merge(&into->env, &other->env);
}

void
fatso_configuration_add_package(struct fatso* f, struct fatso_configuration* config, struct fatso_package* p) {
  merge_configurations(config, &p->base_configuration);

  for (size_t i = 0; i < p->configurations.size; ++i) {
    struct fatso_configuration* c = &p->configurations.data[i];
    // TODO: Only merge it if we're interested in that configuration.
    merge_configurations(config, c);
  }
}
