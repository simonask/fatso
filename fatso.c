#include "fatso.h"

#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

int
fatso_update(struct fatso* f, int argc, char const* argv[]) {
  printf("<UPDATE> %s\n", f->global_dir);
  return 0;
}

static const char*
get_homedir() {
  const char* homedir = getenv("HOME");
  if (homedir == NULL) {
    struct passwd* pw = getpwuid(getuid());
    homedir = pw->pw_dir;
  }
  return homedir;
}

static bool
dir_exists(const char* path) {
  struct stat s;
  int r = lstat(path, &s);

  if (errno == ENOENT || errno == ENOTDIR) {
    return false;
  }

  if (r != 0) {
    perror("lstat");
    exit(1);
  }

  return S_ISDIR(s.st_mode);
}

static bool
file_exists(const char* path) {
  struct stat s;
  int r = lstat(path, &s);

  if (errno == ENOENT || errno == ENOTDIR) {
    return false;
  }

  if (r != 0) {
    perror("lstat");
    exit(1);
  }

  return !(S_ISDIR(s.st_mode));
}

int
fatso_init(struct fatso* f) {
  int r;

  const char* homedir = get_homedir();
  size_t homedir_len = strlen(homedir);
  f->global_dir = malloc(homedir_len + 50);
  strncpy(f->global_dir, homedir, homedir_len);
  strncat(f->global_dir, "/.fatso", 7);

  if (!dir_exists(f->global_dir)) {
    r = mkdir(f->global_dir, 0755);
    if (r != 0) {
      fprintf(stderr, "Fatso homedir (%s) does not exist, and could not be created.\n", f->global_dir);
      perror("mkdir");
      exit(1);
    }
  }

  return 0;
}

void
fatso_destroy(struct fatso* f) {
  free(f->global_dir);
  free(f->local_dir);
}

