#include "internal.h"

#include <string.h>
#include <stdlib.h> // free

void
fatso_dependency_init(struct fatso_dependency* dep, const char* name, struct fatso_version* version, enum fatso_version_requirement version_requirement) {
  dep->name = strdup(name);
  dep->version.components = version->components;
  dep->version.num_components = version->num_components;
  version->components = NULL;
  version->num_components = 0;
  dep->version_requirement = version_requirement;
}

void
fatso_dependency_destroy(struct fatso_dependency* dep) {
  free(dep->name);
  fatso_version_destroy(&dep->version);
}
