#include "fatso.h"
#include "internal.h"
#include "util.h"

#include <getopt.h>
#include <unistd.h> // getcwd
#include <stdio.h>
#include <stdlib.h> // exit, realpath
#include <string.h> // strerror
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h> // mkdir
#include <sys/param.h> // MAXPATHLEN

int
fatso_upgrade(struct fatso* f, int argc, char* const* argv) {
  printf("<UPDATE> %s\n", f->global_dir);
  return 0;
}

static void
log_stdio(struct fatso* f, int level, const char* message, size_t message_len) {
  FILE* fp = stderr;
  if (level == FATSO_LOG_INFO) {
    fp = stdout;
  }
  fwrite(message, message_len, 1, fp);
  fwrite("\n", 1, 1, fp);
}

static struct fatso_logger g_default_logger = { log_stdio };

int
fatso_init(struct fatso* f, const char* program_name) {
  f->program_name = program_name;
  f->project = NULL;
  f->command = NULL;
  f->global_dir = NULL;
  f->working_dir = NULL;
  f->logger = &g_default_logger;
  return 0;
}

void
fatso_set_logger(struct fatso* f, const struct fatso_logger* logger) {
  f->logger = logger;
}

void fatso_set_home_directory(struct fatso* f, const char* path) {
  free(f->global_dir);
  f->global_dir = realpath(path, NULL);
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
      fatso_logf(f, FATSO_LOG_FATAL, "Fatso homedir (%s) does not exist, and could not be created.\nmkdir: %s", f->global_dir, strerror(errno));
      exit(1);
    }
  }

  return f->global_dir;
}

void fatso_set_project_directory(struct fatso* f, const char* path) {
  free(f->working_dir);
  f->working_dir = realpath(path, NULL);
}

static char*
find_fatso_yml(struct fatso* f) {
  char* wd = fatso_alloc(MAXPATHLEN);
  char* r = getcwd(wd, MAXPATHLEN);
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
  fatso_free(wd);
  fatso_free(check);
  return NULL;
}

const char*
fatso_project_directory(struct fatso* f) {
  if (!f->working_dir) {
    f->working_dir = find_fatso_yml(f);
    if (f->working_dir == NULL) {
      fatso_logf(f, FATSO_LOG_FATAL, "fatso.yml not found in this or any parent directory.");
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

void
fatso_logz(struct fatso* f, int level, const char* message, size_t message_len) {
  f->logger->log(f, level, message, message_len);
}

void
fatso_logf(struct fatso* f, int level, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char* message;
  vasprintf(&message, fmt, ap);
  va_end(ap);
  fatso_logz(f, level, message, strlen(message));
  fatso_free(message);
}
