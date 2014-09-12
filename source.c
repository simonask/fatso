#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup
#include <yaml.h>
#include <libgen.h> // basename
#include <unistd.h> // getwd, chdir

struct fatso_source_vtbl {
  const char* type;
  int(*fetch)(struct fatso*, struct fatso_package*, struct fatso_source*);
  int(*unpack)(struct fatso*, struct fatso_package*, struct fatso_source*);
  void(*free)(void* thunk);
};

int
tarball_get_paths(struct fatso* f, struct fatso_package* p, struct fatso_source* source, char** out_directory, char** out_filepath) {
  int r = 0;
  const char* url = source->name;
  char* name = strdup(url); // Need a copy for basename()

  char* filename = basename(name);
  if (filename == NULL) {
    perror("basename");
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

int
fatso_tarball_fetch(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  const char* url = source->name;
  char* source_dir = NULL;
  char* downloaded_file_path = NULL;

  int r = tarball_get_paths(f, package, source, &source_dir, &downloaded_file_path);

  r = fatso_mkdir_p(source_dir);
  if (r != 0) {
    perror("mkdir");
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

int
fatso_tarball_unpack(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  char* source_dir = NULL;
  char* downloaded_file_path = NULL;
  char* build_path = NULL;
  char* command = NULL;
  char* oldwd = NULL;
  int r = tarball_get_paths(f, package, source, &source_dir, &downloaded_file_path);
  if (r != 0)
    goto out;

  if (!fatso_file_exists(downloaded_file_path)) {
    fprintf(stderr, "File not found: %s\n", downloaded_file_path);
    r = 1;
    goto out;
  }

  build_path = fatso_package_build_path(f, package);
  r = fatso_mkdir_p(build_path);
  if (r != 0) {
    perror(build_path);
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
  return 1;
}

static const struct fatso_source_vtbl tarball_source_vtbl = {
  .type = "tarball",
  .fetch = fatso_tarball_fetch,
  .unpack = fatso_tarball_unpack,
  .free = fatso_free,
};

void
fatso_tarball_init(struct fatso_source* source, const char* url) {
  source->name = strdup(url);
  source->vtbl = &tarball_source_vtbl;
  source->thunk = NULL;
}

int
fatso_source_parse(
  struct fatso_source* source,
  struct yaml_document_s* doc,
  struct yaml_node_s* node,
  char** out_error_message
) {
  if (node->type == YAML_SCALAR_NODE) {
    char* url = fatso_yaml_scalar_strdup(node);
    fatso_tarball_init(source, url);
    fatso_free(url);
    return 0;
  } else {
    *out_error_message = strdup("Invalid source (expected URL).");
    return 1;
  }
}

void
fatso_source_destroy(struct fatso_source* source) {
  source->vtbl->free(source->thunk);
  fatso_free(source->name);
}

int
fatso_source_fetch(struct fatso* f, struct fatso_package* p, struct fatso_source* source) {
  return source->vtbl->fetch(f, p, source);
}

int
fatso_source_unpack(struct fatso* f, struct fatso_package* p, struct fatso_source* source) {
  return source->vtbl->unpack(f, p, source);
}


