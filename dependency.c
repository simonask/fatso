#include "internal.h"

#include <string.h>
#include <stdlib.h> // free

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
