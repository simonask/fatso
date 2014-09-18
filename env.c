#include "internal.h"
#include "fatso.h"

#include <stdlib.h> // putenv, getenv
#include <string.h> // strdup

extern char const* const* environ;

static void
prepend_env_pathlist(const char* name, const char* value) {
  char* current_value = getenv(name);
  if (current_value) {
    char* new_value;
    asprintf(&new_value, "%s:%s", value, current_value);
    setenv(name, new_value, 1);
    fatso_free(new_value);
  } else {
    setenv(name, value, 1);
  }
}

int
fatso_append_base_environment(struct fatso* f) {
  const char* project_dir = fatso_project_directory(f);
  char* bin_path = NULL;
  char* lib_path = NULL;

  // PATH
  asprintf(&bin_path, "%s/.fatso/bin", project_dir);
  prepend_env_pathlist("PATH", bin_path);

  asprintf(&lib_path, "%s/.fatso/lib", project_dir);
#if defined(__APPLE__)
  // DYLD_LIBRARY_PATH
  prepend_env_pathlist("DYLD_LIBRARY_PATH", lib_path);
#elif defined(__linux__)
  // LD_LIBRARY_PATH
  prepend_env_pathlist("LD_LIBRARY_PATH", lib_path);
#else
  #warning Don't know how to inform environment of lib path on this platform. :(
#endif

  fatso_free(bin_path);
  fatso_free(lib_path);
  return 0;
}

int
fatso_package_append_environment(struct fatso* f, struct fatso_package* p) {
  // TODO!
  return 0;
}

int
fatso_load_environment(struct fatso* f) {
  int r;

  r = fatso_append_base_environment(f);
  if (r != 0)
    goto out;

  for (size_t i = 0; i < f->project->install_order.size; ++i) {
    struct fatso_package* p = f->project->install_order.data[i];
    r = fatso_package_append_environment(f, p);
    if (r != 0) {
      goto out;
    }
  }

out:
  return r;
}

static void
print_environment() {
  for (char const* const* p = environ; *p; ++p) {
    printf("%s\n", *p);
  }
}

int fatso_env(struct fatso* f, int argc, char* const* argv) {
  int r;

  r = fatso_load_project(f);
  if (r != 0)
    goto out;

  r = fatso_load_or_generate_dependency_graph(f);
  if (r != 0)
    goto out;

  r = fatso_load_environment(f);
  if (r != 0)
    goto out;

  print_environment();
out:
  return r;
}
