#include "util.h"

#include <sys/stat.h> // lstat
#include <unistd.h>   // getuid
#include <stdlib.h>   // getenv
#include <errno.h>    // errno
#include <stdio.h>    // perror
#include <pwd.h>      // getpwuid


const char*
fatso_get_homedir() {
  const char* homedir = getenv("HOME");
  if (homedir == NULL) {
    struct passwd* pw = getpwuid(getuid());
    homedir = pw->pw_dir;
  }
  return homedir;
}

bool
fatso_directory_exists(const char* path) {
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

bool
fatso_file_exists(const char* path) {
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
