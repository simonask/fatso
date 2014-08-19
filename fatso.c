#include "fatso.h"
#include "util.h"

#include <getopt.h>
#include <unistd.h> // getwd
#include <stdio.h>
#include <stdlib.h> // malloc, free, exit
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h> // mkdir

int
fatso_update(struct fatso* f, int argc, char const* argv[]) {
  printf("<UPDATE> %s\n", f->global_dir);
  return 0;
}

int
fatso_init(struct fatso* f, const char* program_name) {
  int r;

  f->program_name = program_name;
  f->project = NULL;
  f->command = NULL;

  const char* homedir = fatso_get_homedir();
  size_t homedir_len = strlen(homedir);
  f->global_dir = malloc(homedir_len + 50);
  strncpy(f->global_dir, homedir, homedir_len);
  strncat(f->global_dir, "/.fatso", 7);

  if (!fatso_directory_exists(f->global_dir)) {
    r = mkdir(f->global_dir, 0755);
    if (r != 0) {
      fprintf(stderr, "Fatso homedir (%s) does not exist, and could not be created.\n", f->global_dir);
      perror("mkdir");
      exit(1);
    }
  }

  // TODO: Adhere to -C option.
  f->working_dir = getwd(NULL);

  return 0;
}

void
fatso_destroy(struct fatso* f) {
  free(f->project);
  free(f->global_dir);
}

int
fatso_update_packages(struct fatso* f) {
  int r = 0;
  char* cmd = NULL;
  char* packages_dir = NULL;
  char* packages_git_dir = NULL;

  asprintf(&packages_dir, "%s/packages", f->global_dir);
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
    asprintf(&cmd, "git -C %s clone file:///Users/simon/code/fatso-packages/.git packages", f->global_dir);
    r = system(cmd);
    if (r != 0) {
      fprintf(stderr, "git command failed with status %d: %s\nTry to fix it with `fatso doctor` maybe?\n", r, cmd);
      goto out;
    }
  }

out:
  free(packages_dir);
  free(packages_git_dir);
  free(cmd);
  return r;
}

