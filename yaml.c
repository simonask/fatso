#include "internal.h"

#include <yaml.h>
#include <assert.h>

yaml_node_t*
fatso_yaml_mapping_lookup(yaml_document_t* doc, yaml_node_t* mapping, const char* key) {
  assert(mapping->type == YAML_MAPPING_NODE);
  for (yaml_node_pair_t* x = mapping->data.mapping.pairs.start; x != mapping->data.mapping.pairs.top; ++x) {
    yaml_node_t* k = yaml_document_get_node(doc, x->key);
    if (k) {
      if (strncmp(key, (char*)k->data.scalar.value, k->data.scalar.length) == 0) {
        return yaml_document_get_node(doc, x->value);
      }
    }
  }
  return NULL;
}

yaml_node_t*
fatso_yaml_traverse(yaml_document_t* doc, yaml_node_t* node, const char *keys[], size_t num_keys) {
  yaml_node_t* p = node;
  for (size_t i = 0; i < num_keys; ++i) {
    if (p->type == YAML_MAPPING_NODE) {
      p = fatso_yaml_mapping_lookup(doc, p, keys[i]);
      if (p == NULL) {
        break;
      }
    } else {
      return NULL;
    }
  }
  return p;
}

char*
fatso_yaml_scalar_strdup(yaml_node_t* node) {
  if (node) {
    assert(node->type == YAML_SCALAR_NODE);
    return strndup((char*)node->data.scalar.value, node->data.scalar.length);
  }
  return strdup("");
}

size_t
fatso_yaml_sequence_length(yaml_node_t* node) {
  if (node->type != YAML_SEQUENCE_NODE)
    return 0;
  return node->data.sequence.items.top - node->data.sequence.items.start;
}

size_t
fatso_yaml_mapping_length(yaml_node_t* node) {
  if (node->type != YAML_MAPPING_NODE)
    return 0;
  return node->data.mapping.pairs.top - node->data.mapping.pairs.start;
}

yaml_node_t*
fatso_yaml_sequence_lookup(yaml_document_t* doc, yaml_node_t* sequence, size_t idx) {
  assert(sequence->type == YAML_SEQUENCE_NODE);
  if (idx >= fatso_yaml_sequence_length(sequence))
    return NULL;
  return yaml_document_get_node(doc, sequence->data.sequence.items.start[idx]);
}
