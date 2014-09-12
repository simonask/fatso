#include "fatso.h"
#include "internal.h"
#include "util.h"

#include <stdio.h> // asprintf, fprintf, perror
#include <sys/stat.h> // mkdir
#include <stdlib.h> // system

int
fatso_sync_packages(struct fatso* f) {
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
    asprintf(&cmd, "git -C \"%s\" pull", packages_dir);
    r = fatso_system(cmd);
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

int fatso_sync(struct fatso* f, int argc, char* const* argv) {
  return fatso_sync_packages(f);
}
