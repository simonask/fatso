#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup

struct git_data {
  char* url;
  char* ref;
};

static void
git_free(void* thunk) {
  struct git_data* data = thunk;
  fatso_free(data->url);
  fatso_free(data->ref);
  fatso_free(data);
}

static int
git_clone_or_pull(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  int r;
  struct git_data* data = source->thunk;
  char* clone_path;
  asprintf(&clone_path, "%s/sources/%s/git", fatso_home_directory(f), package->name);

  char* cmd = NULL;

  if (fatso_directory_exists(clone_path)) {
    // Run as `git pull`
    asprintf(&cmd, "git -C %s fetch", clone_path);
  } else {
    // Run as `git clone`
    asprintf(&cmd, "git clone --mirror %s %s", data->url, clone_path);
  }

  r = fatso_system_defer_output_until_error(cmd);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "git command failed: %s", cmd);
    goto out;
  }
out:
  fatso_free(clone_path);
  fatso_free(cmd);
  return r;
}

static int
git_checkout(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  int r;
  struct git_data* data = source->thunk;
  char* cmd;
  char* clone_path;
  char* build_path = fatso_package_build_path(f, package);
  asprintf(&clone_path, "%s/sources/%s/git", fatso_home_directory(f), package->name);

  if (fatso_directory_exists(build_path)) {
    asprintf(&cmd, "rm -rf %s", build_path);
    r = fatso_system_defer_output_until_error(cmd);
    if (r != 0) {
      fatso_logf(f, FATSO_LOG_FATAL, "Could not remove existing checkout.");
      goto out;
    }
    fatso_free(cmd);
  }

  asprintf(&cmd, "git clone file://%s %s --recursive --depth 1 --branch %s", clone_path, build_path, data->ref);
  r = fatso_system_defer_output_until_error(cmd);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "git command failed: %s", cmd);
    goto out;
  }
  fatso_free(cmd);

  asprintf(&cmd, "git -C %s checkout %s", build_path, data->ref);
  r = fatso_system_defer_output_until_error(cmd);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "git command failed: %s", cmd);
    goto out;
  }

out:
  fatso_free(clone_path);
  fatso_free(cmd);
  fatso_free(build_path);
  return r;
}

static const struct fatso_source_vtbl git_source_vtbl = {
  .type = "git",
  .fetch = git_clone_or_pull,
  .unpack = git_checkout,
  .free = git_free,
};

void
fatso_git_source_init(struct fatso_source* source, const char* url, const char* ref) {
  asprintf(&source->name, "%s@%s", url, ref);
  source->vtbl = &git_source_vtbl;
  source->thunk = fatso_alloc(sizeof(struct git_data));
  struct git_data* data = source->thunk;
  data->url = strdup(url);
  data->ref = strdup(ref);
}
