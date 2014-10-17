#include "fatso.h"
#include "internal.h"

typedef int(*init_toolchain_t)(struct fatso_toolchain*);
typedef bool(*guess_toolchain_t)(const char* path);

struct init_named_toolchain {
  const char* name;
  init_toolchain_t init;
  guess_toolchain_t guess;
};

static const struct init_named_toolchain named_toolchains[] = {
  {"autotools", fatso_init_toolchain_autotools_make, fatso_path_looks_like_autotools_project},
  {"configure_and_make", fatso_init_toolchain_configure_and_make, fatso_path_looks_like_configure_and_make_project},
  {"make", fatso_init_toolchain_plain_make, fatso_path_looks_like_plain_make_project},
  // {"cmake", fatso_init_toolchain_cmake},
  {"scons", fatso_init_toolchain_scons, fatso_path_looks_like_scons_project},
  // {"bjam", fatso_init_toolchain_bjam},
  {NULL, NULL}
};

int
fatso_guess_toolchain(struct fatso* f, struct fatso_package* p, struct fatso_toolchain* out_toolchain) {
  // TODO: Use 'toolchain' option in package.

  char* path = fatso_package_build_path(f, p);
  bool found = false;
  for (const struct init_named_toolchain* i = named_toolchains; i->name; ++i) {
    if (i->guess(path)) {
      found = true;
      i->init(out_toolchain);
    }
  }
  fatso_free(path);
  if (!found) {
    fatso_logf(f, FATSO_LOG_FATAL, "Fatso couldn't guess the toolchain of package '%s', and none was explicitly defined!", p->name);
    return 1;
  }
  return 0;
}
