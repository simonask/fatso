#include "fatso.h"
#include "util.h"

#include <yaml.h>

struct fatso_project {
  char* path;
};

static char*
find_fatso_yml(const char* working_dir) {
  char* r = strdup(working_dir);
  char* check;
  asprintf(&check, "%s/fatso.yml", r);

  while (!fatso_file_exists(check)) {
    char* p = strrchr(r, '/');
    if (p == r || p == NULL) {
      goto not_found;
    }
    *p = '\0';
    sprintf(check, "%s/fatso.yml", r);
  }
out:
  free(check);
  return r;
not_found:
  free(r);
  free(check);
  return NULL;
}

int
fatso_load_project(struct fatso* f) {
  int r = 0;

  if (f->project != NULL) {
    // Project is already loaded.
    goto good;
  }

  f->project = malloc(sizeof(struct fatso_project));
  f->project->path = find_fatso_yml(f->working_dir);

  if (f->project->path) {
    printf("DEBUG: fatso.yml found in %s\n", f->project->path);
  } else {
    printf("DEBUG: fatso.yml NOT FOUND!\n");
    goto error;
  }

  // Find nearest fatso.yml
  // Read it into the fatso_project struct (while checking for consistency)

good:
  return r;
error:
  fatso_unload_project(f);
  return r;
}

void
fatso_unload_project(struct fatso* f) {
  free(f->project->path);
  free(f->project);
  f->project = NULL;
}

int
fatso_load_or_generate_dependency_graph(struct fatso* f) {
  int r;

  r = fatso_load_dependency_graph(f);
  if (r != 0) {
    r = fatso_generate_dependency_graph(f);
  }
  return r;
}

int
fatso_load_dependency_graph(struct fatso* f) {
  fprintf(stderr, "%s: NIY\n", __func__);
  return -1;
}

int fatso_generate_dependency_graph(struct fatso* f) {
  fprintf(stderr, "%s: NIY\n", __func__);
  return -1;
}
