#include "fatso.h"
#include "internal.h"
#include "util.h"

#include <getopt.h>
#include <unistd.h> // getwd
#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h> // mkdir

int
fatso_upgrade(struct fatso* f, int argc, char* const* argv) {
  printf("<UPDATE> %s\n", f->global_dir);
  return 0;
}

int
fatso_init(struct fatso* f, const char* program_name) {
  f->program_name = program_name;
  f->project = NULL;
  f->command = NULL;
  f->global_dir = NULL;
  return 0;
}

void fatso_set_home_directory(struct fatso* f, const char* path) {
  free(f->global_dir);
  f->global_dir = strdup(path);
}

const char*
fatso_home_directory(struct fatso* f) {
  if (!f->global_dir) {
    const char* homedir = fatso_get_homedir();
    size_t homedir_len = strlen(homedir);
    f->global_dir = fatso_alloc(homedir_len + 50);
    strncpy(f->global_dir, homedir, homedir_len);
    strncat(f->global_dir, "/.fatso", 7);
  }

  if (!fatso_directory_exists(f->global_dir)) {
    int r = mkdir(f->global_dir, 0755);
    if (r != 0) {
      fprintf(stderr, "Fatso homedir (%s) does not exist, and could not be created.\n", f->global_dir);
      perror("mkdir");
      exit(1);
    }
  }

  return f->global_dir;
}

void fatso_set_project_directory(struct fatso* f, const char* path) {
  free(f->working_dir);
  f->working_dir = strdup(path);
}

static char*
find_fatso_yml(struct fatso* f) {
  char* r = getwd(NULL);
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
  fatso_free(check);
  return r;
not_found:
  fatso_free(r);
  fatso_free(check);
  return NULL;
}

const char*
fatso_project_directory(struct fatso* f) {
  if (!f->working_dir) {
    f->working_dir = find_fatso_yml(f);
    if (f->working_dir == NULL) {
      fprintf(stderr, "fatso.yml not found in this or any parent directory.\n");
      exit(1);
    }
  }
  return f->working_dir;
}

void
fatso_destroy(struct fatso* f) {
  fatso_free(f->project);
  fatso_free(f->global_dir);
  fatso_free(f->working_dir);
}
