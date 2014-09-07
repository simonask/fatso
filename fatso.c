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
fatso_update(struct fatso* f, int argc, char* const* argv) {
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

void fatso_set_working_directory(struct fatso* f, const char* path) {
  free(f->working_dir);
  f->working_dir = strdup(path);
}

const char*
fatso_working_directory(struct fatso* f) {
  if (!f->working_dir) {
    f->working_dir = getwd(NULL);
  }
  return f->working_dir;
}

void
fatso_destroy(struct fatso* f) {
  fatso_free(f->project);
  fatso_free(f->global_dir);
  fatso_free(f->working_dir);
}

int
fatso_update_packages(struct fatso* f) {
  int r = 0;
  char* cmd = NULL;
  char* packages_dir = NULL;
  char* packages_git_dir = NULL;

  asprintf(&packages_dir, "%s/packages", fatso_home_directory(f));
  asprintf(&packages_git_dir, "%s/.git", packages_dir);

  if (!fatso_directory_exists(packages_dir)) {
    r = mkdir(packages_dir, 0755);
    if (r != 0) {
      fprintf(stderr, "Could not create dir %s, aborting.", packages_dir);
      perror("mkdir");
      goto out;
    }
  }

  fprintf(stdout, "Updating Fatso packages...\n");
  if (fatso_directory_exists(packages_git_dir)) {
    asprintf(&cmd, "git -C %s pull", packages_dir);
    r = system(cmd);
    if (r != 0) {
      fprintf(stderr, "git command failed with status %d: %s\nTry to fix it with `fatso doctor` maybe?\n", r, cmd);
      goto out;
    }
  } else {
    asprintf(&cmd, "git -C %s clone http://github.com/simonask/fatso-packages packages", f->global_dir);
    r = system(cmd);
    if (r != 0) {
      fprintf(stderr, "git command failed with status %d: %s\nTry to fix it with `fatso doctor` maybe?\n", r, cmd);
      goto out;
    }
  }

out:
  fatso_free(packages_dir);
  fatso_free(packages_git_dir);
  fatso_free(cmd);
  return r;
}
