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
  dep->num_constraints = num_constraints;
  dep->constraints = fatso_calloc(num_constraints, sizeof(struct fatso_constraint));
  for (size_t i = 0; i < num_constraints; ++i) {
    fatso_version_init(&dep->constraints[i].version);
    fatso_version_copy(&dep->constraints[i].version, &constraints[i].version);
    dep->constraints[i].version_requirement = constraints[i].version_requirement;
  }
}

void
fatso_dependency_destroy(struct fatso_dependency* dep) {
  fatso_free(dep->name);
  for (size_t i = 0; i < dep->num_constraints; ++i) {
    fatso_constraint_destroy(&dep->constraints[i]);
  }
  fatso_free(dep->constraints);
  dep->name = NULL;
  dep->constraints = NULL;
}
