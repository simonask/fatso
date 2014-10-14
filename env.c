#include "internal.h"
#include "fatso.h"

#include <stdlib.h> // putenv, getenv
#include <string.h> // strdup

extern char** environ;

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

static void
append_env_words(const char* name, const char* value) {
  char* current_value = getenv(name);
  if (current_value) {
    char* new_value;
    asprintf(&new_value, "%s %s", current_value, value);
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
  char* cflags = NULL;
  char* ldflags = NULL;

  // PATH
  asprintf(&bin_path, "%s/.fatso/bin", project_dir);
  prepend_env_pathlist("PATH", bin_path);

  asprintf(&lib_path, "%s/.fatso/lib", project_dir);
#if defined(__APPLE__)
  // DYLD_LIBRARY_PATH
  prepend_env_pathlist("DYLD_LIBRARY_PATH", lib_path);
#elif defined(__linux)
  // LD_LIBRARY_PATH
  prepend_env_pathlist("LD_LIBRARY_PATH", lib_path);
#else
  #warning "Don't know how to inform environment of lib path on this platform. :("
#endif

  // CFLAGS
  asprintf(&cflags, "-I%s/.fatso/include", project_dir);
  append_env_words("CFLAGS", cflags);
  append_env_words("CXXFLAGS", cflags);

  // LDFLAGS
  asprintf(&ldflags, "-L%s/.fatso/lib", project_dir);
  append_env_words("LDFLAGS", ldflags);


  fatso_free(bin_path);
  fatso_free(lib_path);
  fatso_free(cflags);
  fatso_free(ldflags);
  return 0;
}

void fatso_env_add_package(struct fatso* f, struct fatso_package* p) {
  fatso_env_add_configuration(f, &p->base_configuration);
  for (size_t i = 0; i < p->configurations.size; ++i) {
    // TODO: Check that configuration is one we're interested in.
    fatso_env_add_configuration(f, &p->configurations.data[i]);
  }
}

int
fatso_load_environment(struct fatso* f) {
  int r;

  r = fatso_append_base_environment(f);
  if (r != 0)
    goto out;

  if (f->project) {
    for (size_t i = 0; i < f->project->install_order.size; ++i) {
      struct fatso_package* p = f->project->install_order.data[i];
      fatso_configuration_add_package(f, f->consolidated_configuration, p);
      fatso_env_add_package(f, p);
    }
    fatso_configuration_add_package(f, f->consolidated_configuration, &f->project->package);
    fatso_env_add_package(f, &f->project->package);
  }

out:
  return r;
}

void
fatso_env_add_configuration(struct fatso* f, const struct fatso_configuration* config) {
  for (size_t i = 0; i < config->env.size; ++i) {
    const struct fatso_kv_pair* pair = &config->env.data[i];

    // TODO: Probably add more special cases?
    if (strncmp("CFLAGS", pair->key, 7) == 0) {
      append_env_words(pair->key, pair->value);
    } else if (strncmp("CXXFLAGS", pair->key, 9) == 0) {
      append_env_words(pair->key, pair->value);
    } else if (strncmp("LDFLAGS", pair->key, 8) == 0) {
      append_env_words(pair->key, pair->value);
    } else if (strncmp("PATH", pair->key, 5) == 0) {
      prepend_env_pathlist(pair->key, pair->value);
    } else {
      setenv(pair->key, pair->value, 1);
    }
  }

  for (size_t i = 0; i < config->defines.size; ++i) {
    char* def;
    asprintf(&def, "-D%s=%s", config->defines.data[i].key, config->defines.data[i].value);
    append_env_words("CFLAGS", def);
    append_env_words("CXXFLAGS", def);
    fatso_free(def);
  }
}

static void
print_environment() {
  for (char** p = environ; *p; ++p) {
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
