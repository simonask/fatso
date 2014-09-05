#include "internal.h"

#include <stdlib.h> // free
#include <yaml.h>

void
fatso_dependency_init(
  struct fatso_dependency* dep,
  const char* name,
  const struct fatso_constraint* constraints,
  size_t num_constraints)
{
  dep->name = strdup(name);
  dep->constraints.size = num_constraints;
  dep->constraints.data = fatso_calloc(num_constraints, sizeof(struct fatso_constraint));
  for (size_t i = 0; i < num_constraints; ++i) {
    fatso_version_init(&dep->constraints.data[i].version);
    fatso_version_copy(&dep->constraints.data[i].version, &constraints[i].version);
    dep->constraints.data[i].version_requirement = constraints[i].version_requirement;
  }
}

void
fatso_dependency_destroy(struct fatso_dependency* dep) {
  fatso_free(dep->name);
  for (size_t i = 0; i < dep->constraints.size; ++i) {
    fatso_constraint_destroy(&dep->constraints.data[i]);
  }
  fatso_free(dep->constraints.data);
  dep->name = NULL;
  dep->constraints.data = NULL;
}

int
fatso_dependency_parse(struct fatso_dependency* dep, struct yaml_document_s* doc, struct yaml_node_s* node, char** out_error_message) {
  int r = 1;
  char* version_requirement_string = NULL;

  if (node->type != YAML_SEQUENCE_NODE || fatso_yaml_sequence_length(node) == 0) {
    *out_error_message = strdup("Invalid dependency, expected a tuple of [package, version].");
    goto out;
  }

  yaml_node_t* package_name = fatso_yaml_sequence_lookup(doc, node, 0);

  yaml_node_t* version_or_options = fatso_yaml_sequence_lookup(doc, node, 1);
  struct fatso_constraint constraint = {{0}}; // TODO: Support >1 constraint
  if (version_or_options == NULL) {
    fatso_version_from_string(&constraint.version, "");
    constraint.version_requirement = FATSO_VERSION_ANY;
  } else if (version_or_options->type == YAML_SCALAR_NODE) {
    version_requirement_string = fatso_yaml_scalar_strdup(version_or_options);
    struct fatso_constraint constraint;
    r = fatso_constraint_from_string(&constraint, version_requirement_string);
    if (r != 0)
      goto error;
  } else {
    *out_error_message = strdup("Invalid version/options!");
    goto out;
  }

  char* name = fatso_yaml_scalar_strdup(package_name);
  fatso_dependency_init(dep, name, &constraint, 1);
  fatso_free(name);
out:
  fatso_free(version_requirement_string);
  return r;
error:
  fatso_dependency_destroy(dep);
  goto out;
}

struct fatso_dependency_graph {
  FATSO_ARRAY(struct fatso_package*) packages;
};

static int
compare_package_pointers_by_name(const void* a, const void* b) {
  const struct fatso_package* pa = a;
  const struct fatso_package* pb = b;
  return strcmp(pa->name, pb->name);
}

struct fatso_dependency_graph*
fatso_dependency_graph_new() {
  return fatso_alloc(sizeof(struct fatso_dependency_graph));
}

struct fatso_dependency_graph*
fatso_dependency_graph_copy(struct fatso_dependency_graph* graph) {
  return fatso_dependency_graph_new();
}

void
fatso_dependency_graph_free(struct fatso_dependency_graph* graph) {
  // TODO: Destructor
  fatso_free(graph);
}

void
fatso_dependency_graph_add_package(struct fatso_dependency_graph* graph, struct fatso_package* package) {
  fatso_multiset_insert_v(&graph->packages, &package, compare_package_pointers_by_name);
}

enum fatso_dependency_graph_resolution_status
fatso_dependency_graph_resolve(struct fatso_dependency_graph* graph) {
  return FATSO_DEPENDENCY_GRAPH_CONFLICT; // TODO
}

void
fatso_dependency_graph_topological_sort(struct fatso_dependency_graph* graph, struct fatso_package*** out_list, size_t* out_size) {

}
