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

void
fatso_dependency_copy(struct fatso_dependency* dep, const struct fatso_dependency* old) {
  fatso_dependency_init(dep, old->name, old->constraints.data, old->constraints.size);
}

struct fatso_dependency*
fatso_dependency_copy_new(const struct fatso_dependency* old) {
  struct fatso_dependency* dep = fatso_alloc(sizeof(struct fatso_dependency));
  fatso_dependency_copy(dep, old);
  return dep;
}

void
fatso_dependency_add_constraint(struct fatso_dependency* dep, const struct fatso_constraint* constraint) {
  struct fatso_constraint copy = {{0}};
  fatso_version_copy(&copy.version, &constraint->version);
  copy.version_requirement = constraint->version_requirement;
  fatso_push_back_v(&dep->constraints, &copy);
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

struct fatso_reverse_dependency_list {
  unsigned int dependency;
  FATSO_ARRAY(struct fatso_package*) packages;
};

struct fatso_dependency_graph {
  // Shared pointers:
  FATSO_ARRAY(struct fatso_package*) closed_set;
  FATSO_ARRAY(unsigned int) open_set;  // sorted by pointer
  FATSO_ARRAY(unsigned int) conflicts; // sorted by pointer
  FATSO_ARRAY(unsigned int) unknown;   // sorted by pointer

  // Owned pointers, that must be freed iteratively:
  FATSO_ARRAY(char*) topological_order;
  FATSO_ARRAY(struct fatso_dependency) own_dependencies; // sorted by name
  FATSO_ARRAY(struct fatso_reverse_dependency_list) depended_on_by; // sorted by name
};

static void
increment_indices_from(struct fatso_dependency_graph* graph, unsigned int inserted) {
  for (size_t i = 0; i < graph->open_set.size; ++i) {
    if (graph->open_set.data[i] >= inserted) {
      ++graph->open_set.data[i];
    }
  }
  for (size_t i = 0; i < graph->conflicts.size; ++i) {
    if (graph->conflicts.data[i] >= inserted) {
      ++graph->conflicts.data[i];
    }
  }
  for (size_t i = 0; i < graph->unknown.size; ++i) {
    if (graph->unknown.data[i] >= inserted) {
      ++graph->unknown.data[i];
    }
  }
  for (size_t i = 0; i < graph->depended_on_by.size; ++i) {
    if (graph->depended_on_by.data[i].dependency >= inserted) {
      ++graph->depended_on_by.data[i].dependency;
    }
  }
}

#define fatso_copy_array(new, old) \
  (new)->size = (old)->size; \
  (new)->data = fatso_calloc((old)->size, sizeof(*(old)->data)); \
  memcpy((new)->data, (old)->data, sizeof(*(old)->data) * (old)->size)

struct fatso_dependency_graph*
fatso_dependency_graph_copy(struct fatso_dependency_graph* old) {
  struct fatso_dependency_graph* graph = fatso_dependency_graph_new();

  // Shallow is fine, because package pointers are shared.
  fatso_copy_array(&graph->closed_set, &old->closed_set);
  fatso_copy_array(&graph->open_set, &old->open_set);
  fatso_copy_array(&graph->conflicts, &old->conflicts);
  fatso_copy_array(&graph->unknown, &old->unknown);

  // Deep copies:

  fatso_copy_array(&graph->own_dependencies, &old->own_dependencies);
  for (size_t i = 0; i < graph->own_dependencies.size; ++i) {
    fatso_dependency_copy(&graph->own_dependencies.data[i], &old->own_dependencies.data[i]);
  }

  fatso_copy_array(&graph->topological_order, &old->topological_order);
  for (size_t i = 0; i < graph->topological_order.size; ++i) {
    graph->topological_order.data[i] = strdup(old->topological_order.data[i]);
  }

  fatso_copy_array(&graph->depended_on_by, &old->depended_on_by);
  for (size_t i = 0; i < graph->depended_on_by.size; ++i) {
    fatso_copy_array(&graph->depended_on_by.data[i].packages, &old->depended_on_by.data[i].packages);
  }

  return graph;
}

void
fatso_dependency_graph_free(struct fatso_dependency_graph* graph) {
  for (size_t i = 0; i < graph->own_dependencies.size; ++i) {
    fatso_dependency_destroy(&graph->own_dependencies.data[i]);
  }

  for (size_t i = 0; i < graph->topological_order.size; ++i) {
    fatso_free(graph->topological_order.data[i]);
  }

  for (size_t i = 0; i < graph->depended_on_by.size; ++i) {
    fatso_free(graph->depended_on_by.data[i].packages.data);
  }

  fatso_free(graph->closed_set.data);
  fatso_free(graph->open_set.data);
  fatso_free(graph->conflicts.data);
  fatso_free(graph->unknown.data);
  fatso_free(graph->own_dependencies.data);
  fatso_free(graph->topological_order.data);
  fatso_free(graph->depended_on_by.data);

  fatso_free(graph);
}

static int
compare_pointers(const void* ppa, const void* ppb) {
  return memcmp(ppa, ppb, sizeof(void*));
}

static int
compare_package_pointers_by_name(const void* ppa, const void* ppb) {
  const struct fatso_package* pa = *(void**)ppa;
  const struct fatso_package* pb = *(void**)ppb;
  return strcmp(pa->name, pb->name);
}

static int
compare_dependency_pointers_by_name(const void* ppa, const void* ppb) {
  const struct fatso_dependency* pa = *(void**)ppa;
  const struct fatso_dependency* pb = *(void**)ppb;
  return strcmp(pa->name, pb->name);
}

static int
compare_uints(const void* a, const void* b) {
  return *(int*)a - *(int*)b;
}

static int
compare_reverse_dependency_lists_by_dependency_index(const void* pa, const void* pb) {
  const struct fatso_reverse_dependency_list* a = pa;
  const struct fatso_reverse_dependency_list* b = pb;
  return (int)a->dependency - (int)b->dependency;
}

static int
compare_string_with_package_pointer(const void* str, const void* pb) {
  const struct fatso_package* const* pp = pb;
  return strcmp(str, (*pp)->name);
}

int
fatso_dependency_graph_add_closed_set(
  struct fatso_dependency_graph* graph,
  struct fatso_package* p
) {
  fatso_set_insert_v(&graph->closed_set, &p, compare_package_pointers_by_name);
  return 0;
}

void
fatso_dependency_graph_register_dependency(
  struct fatso_dependency_graph* graph,
  unsigned int dep_idx,
  struct fatso_package* dependency_of
) {
  if (dependency_of == NULL)
    return;

  struct fatso_reverse_dependency_list initial = {
    .dependency = dep_idx,
    .packages = {0},
  };

  struct fatso_reverse_dependency_list* list;
  list = fatso_set_insert_v(&graph->depended_on_by, &initial, compare_reverse_dependency_lists_by_dependency_index);

  fatso_set_insert_v(&list->packages, &dependency_of, compare_pointers);
}

void
fatso_dependency_graph_add_conflict(
  struct fatso_dependency_graph* graph,
  unsigned int dep_idx
) {
  fatso_set_insert_v(&graph->conflicts, &dep_idx, compare_pointers);
}

void
fatso_dependency_graph_add_unknown(
  struct fatso_dependency_graph* graph,
  unsigned int dep_idx
) {
  fatso_set_insert_v(&graph->unknown, &dep_idx, compare_pointers);
}

static const int FATSO_DEPENDENCY_OK = 0;
static const int FATSO_DEPENDENCY_BLOCKED = 2;

int
fatso_dependency_graph_add_open_set(
  struct fatso_dependency_graph* graph,
  struct fatso_dependency* dep,
  struct fatso_package* dependency_of
) {
  struct fatso_dependency** existing_owned_dependency_p;
  existing_owned_dependency_p = fatso_bsearch_v(&dep, &graph->own_dependencies, compare_dependency_pointers_by_name);

  // We've never seen this before, so it can't go wrong.
  if (existing_owned_dependency_p == NULL) {
    struct fatso_dependency duplicate;
    fatso_dependency_copy(&duplicate, dep);
    struct fatso_dependency* inserted = fatso_set_insert_v(&graph->own_dependencies, &duplicate, compare_dependency_pointers_by_name);
    unsigned int inserted_index = inserted - graph->own_dependencies.data;
    increment_indices_from(graph, inserted_index);
    fatso_set_insert_v(&graph->open_set, &inserted_index, compare_uints);
    fatso_dependency_graph_register_dependency(graph, inserted_index, dependency_of);
    return FATSO_DEPENDENCY_OK;
  }

  struct fatso_dependency* existing_dep = *existing_owned_dependency_p;
  unsigned int existing_index = existing_dep - graph->own_dependencies.data;
  fatso_dependency_graph_register_dependency(graph, existing_index, dependency_of);

  // We've seen it before, so check that it's not in the closed set.
  struct fatso_package** existing_package_p;
  existing_package_p = fatso_bsearch_v(dep->name, &graph->closed_set, compare_string_with_package_pointer);
  if (existing_package_p == NULL) {
    // It's not in the closed set -- add the constraints from the incoming dependency.
    for (size_t i = 0; i < dep->constraints.size; ++i) {
      fatso_dependency_add_constraint(existing_dep, &dep->constraints.data[i]);
    }
    return FATSO_DEPENDENCY_OK;
  } else {
    // It's in the closed set -- check that the pinned version matches this dependency.
    struct fatso_package* package = *existing_package_p;
    if (fatso_version_matches_constraints(&package->version, dep->constraints.data, dep->constraints.size)) {
      return FATSO_DEPENDENCY_OK;
    } else {
      fatso_dependency_graph_add_conflict(graph, existing_index);
      return FATSO_DEPENDENCY_BLOCKED;
    }
  }
}

struct fatso_dependency_graph*
fatso_dependency_graph_new() {
  return fatso_alloc(sizeof(struct fatso_dependency_graph));
}

int
fatso_dependency_graph_add_dependencies_from_package(
  struct fatso_dependency_graph* graph,
  struct fatso* f,
  struct fatso_package* package
) {
  for (size_t i = 0; i < package->base_environment.dependencies.size; ++i) {
    struct fatso_dependency* dep = &package->base_environment.dependencies.data[i];
    int r = fatso_dependency_graph_add_open_set(graph, dep, package);
    if (r != FATSO_DEPENDENCY_OK) {
      return r;
    }
  }

  // TODO: Add dependencies from additional environments.
  return FATSO_DEPENDENCY_OK;
}

struct fatso_dependency_graph*
fatso_dependency_graph_resolve(
  struct fatso* f,
  struct fatso_dependency_graph* graph,
  enum fatso_dependency_graph_resolution_status* out_status
) {
  if (graph->open_set.size == 0) {
    *out_status = FATSO_DEPENDENCY_GRAPH_SUCCESS;
    return graph;
  } else {
    // Pop a dependency off the end of the open set:
    unsigned int dep_idx = graph->open_set.data[graph->open_set.size - 1];
    --graph->open_set.size;
    struct fatso_dependency* dep = &graph->own_dependencies.data[dep_idx];

    // For each package version matching the constraints, starting at the newest,
    // try to append the
    enum fatso_repository_result q;
    struct fatso_package* package = NULL;
    struct fatso_dependency_graph* candidate = NULL;
    while (true) {
      // Find the newest version that's older than the one we've already seen:
      q = fatso_repository_find_package_matching_dependency(f, dep, package ? &package->version : NULL, &package);

      switch (q) {
        case FATSO_PACKAGE_UNKNOWN: {
          fatso_dependency_graph_add_unknown(graph, dep_idx);
          *out_status = FATSO_DEPENDENCY_GRAPH_UNKNOWN;
          return candidate;
        }
        case FATSO_PACKAGE_NO_MATCHING_VERSION: {
          // No more matching versions.
          // If we had a candidate, register conflicts in that.
          *out_status = FATSO_DEPENDENCY_GRAPH_CONFLICT;
          if (candidate) {
            return candidate;
          } else {
            fatso_dependency_graph_add_conflict(graph, dep_idx);
            return graph;
          }
        }
        case FATSO_PACKAGE_OK: {
          // Found a package, clean up old candidates:
          if (candidate) {
            fatso_dependency_graph_free(candidate);
            candidate = NULL;
          }

          candidate = fatso_dependency_graph_copy(graph);
          fatso_dependency_graph_add_closed_set(candidate, package);

          int r = fatso_dependency_graph_add_dependencies_from_package(candidate, f, package);
          if (r == FATSO_DEPENDENCY_OK) {
            struct fatso_dependency_graph* subcandidate;
            subcandidate = fatso_dependency_graph_resolve(f, candidate, out_status);
            if (subcandidate != candidate) {
              fatso_dependency_graph_free(candidate);
              candidate = subcandidate;
            }

            // Add the package to the topologically sorted list (i.e., after its dependencies):
            char* name = strdup(dep->name);
            fatso_push_back_v(&candidate->topological_order, &name);

            if (*out_status == FATSO_DEPENDENCY_GRAPH_SUCCESS) {
              return candidate;
            }
          }
          break;
        }
      }
    }
  }
}

struct fatso_dependency_graph*
fatso_dependency_graph_for_package(
  struct fatso* f,
  struct fatso_package* package,
  enum fatso_dependency_graph_resolution_status* out_status
) {
  struct fatso_dependency_graph* graph = fatso_dependency_graph_new();
  fatso_dependency_graph_add_closed_set(graph, package);

  int r = fatso_dependency_graph_add_dependencies_from_package(graph, f, package);
  if (r == FATSO_DEPENDENCY_OK) {
    struct fatso_dependency_graph* candidate = fatso_dependency_graph_resolve(f, graph, out_status);
    if (candidate != graph) {
      fatso_dependency_graph_free(graph);
    }
    return candidate;
  } else {
    return graph;
  }
}

void
fatso_dependency_graph_topological_sort(struct fatso_dependency_graph* graph, struct fatso_package*** out_list, size_t* out_size) {
  FATSO_ARRAY(struct fatso_package*) list = {
    .data = *out_list,
    .size = *out_size,
  };

  for (size_t i = 0; i < graph->topological_order.size; ++i) {
    const char* name = graph->topological_order.data[i];
    struct fatso_package** package_p = fatso_bsearch_v(name, &graph->closed_set, compare_string_with_package_pointer);
    fatso_push_back_v(&list, package_p);
  }

  *out_list = list.data;
  *out_size = list.size;
}
