#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup, strerror
#include <errno.h>
#include <libgen.h> // basename
#include <unistd.h> // getwd, chdir

static int
tarball_get_paths(struct fatso* f, struct fatso_package* p, struct fatso_source* source, char** out_directory, char** out_filepath) {
  int r = 0;
  const char* url = source->name;
  char* name = strdup(url); // Need a copy for basename()

  char* filename = basename(name);
  if (filename == NULL) {
    fatso_logf(f, FATSO_LOG_FATAL, "basename (%s): %s", name, strerror(errno));
    goto error;
  }

  asprintf(out_directory, "%s/sources/%s/%s", fatso_home_directory(f), p->name, fatso_version_string(&p->version));
  asprintf(out_filepath, "%s/%s", *out_directory, filename);
out:
  fatso_free(name);
  return r;
error:
  r = 1;
  goto out;
}

static int
tarball_fetch(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  const char* url = source->name;
  char* source_dir = NULL;
  char* downloaded_file_path = NULL;

  int r = tarball_get_paths(f, package, source, &source_dir, &downloaded_file_path);

  r = fatso_mkdir_p(source_dir);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "mkdir: %s", strerror(errno));
    goto out;
  }

  if (!fatso_file_exists(downloaded_file_path)) {
    r = fatso_download(downloaded_file_path, url);
    if (r != 0) {
      goto out;
    }
  }

out:
  fatso_free(source_dir);
  fatso_free(downloaded_file_path);
  return r;
}

static int
tarball_unpack(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  char* source_dir = NULL;
  char* downloaded_file_path = NULL;
  char* build_path = NULL;
  char* command = NULL;
  char* oldwd = NULL;
  int r = tarball_get_paths(f, package, source, &source_dir, &downloaded_file_path);
  if (r != 0)
    goto out;

  if (!fatso_file_exists(downloaded_file_path)) {
    fatso_logf(f, FATSO_LOG_FATAL, "File not found: %s", downloaded_file_path);
    r = 1;
    goto out;
  }

  build_path = fatso_package_build_path(f, package);
  r = fatso_mkdir_p(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "mkdir (%s): %s", build_path, strerror(errno));
    goto out;
  }

  asprintf(&command, "tar xf \"%s\" -C \"%s\" --strip-components=1", downloaded_file_path, build_path);
  r = fatso_system(command);
  if (r != 0) {
    goto out;
  }

out:
  fatso_free(oldwd);
  fatso_free(command);
  fatso_free(source_dir);
  fatso_free(downloaded_file_path);
  fatso_free(build_path);
  return r;
}

static const struct fatso_source_vtbl tarball_source_vtbl = {
  .type = "tarball",
  .fetch = tarball_fetch,
  .unpack = tarball_unpack,
  .free = fatso_free,
};

void
fatso_tarball_source_init(struct fatso_source* source, const char* url) {
  source->name = strdup(url);
  source->vtbl = &tarball_source_vtbl;
  source->thunk = NULL;
}
