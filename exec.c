#include "internal.h"
#include "fatso.h"
#include "util.h"

#include <string.h> // strdup

int
fatso_exec(struct fatso* f, int argc, char* const* argv) {
  int r;

  r = fatso_load_project(f);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_WARN, "Warning: No project loaded.");
  } else {
    r = fatso_load_or_generate_dependency_graph(f);
    if (r != 0) {
      fatso_logf(f, FATSO_LOG_WARN, "Warning: Error generating dependency graph.");
    }
  }

  r = fatso_load_environment(f);
  if (r != 0) {
    goto out;
  }

  if (argc == 0) {
    fatso_help(f, argc, argv);
  }

  char* command = strdup(argv[0]);
  for (int i = 1; i < argc; ++i) {
    char* new_command;
    asprintf(&new_command, "%s %s", command, argv[i]);
    fatso_free(command);
    command = new_command;
  }

  r = fatso_system(command);
  fatso_free(command);

out:
  return r;
}
