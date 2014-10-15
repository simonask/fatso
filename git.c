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
git_clone(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  return -1;
}

static int
git_checkout(struct fatso* f, struct fatso_package* package, struct fatso_source* source) {
  return -1;
}

static const struct fatso_source_vtbl git_source_vtbl = {
  .type = "git",
  .fetch = git_clone,
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
