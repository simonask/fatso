#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup
#include <stdlib.h> // system

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



static int build_provided_toolchain( struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* stdio_callbacks){

  if( p->toolchain == NULL ){
    // error
    return -1;
  }

  int r = 0;

  progress(f, p, p->toolchain, 0, 1);
  r = fatso_system_with_callbacks( p->toolchain, stdio_callbacks );
  if( r != 0 ){
    fatso_logf(f, FATSO_LOG_FATAL, "Error during %s.", p->toolchain);
    goto out;
  }
  progress(f, p, p->toolchain, 1, 1);

out:

  return r;
}

static int install_provided_toolchain( struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* stdio_callbacks){

  if( p->toolchain == NULL ){
    // error
    return -1;
  }

  int r = 0;

  progress(f, p, p->toolchain, 0, 1);
  r = fatso_system_with_callbacks( p->toolchain, stdio_callbacks );
  if( r != 0 ){
    fatso_logf(f, FATSO_LOG_FATAL, "Error during %s.", p->toolchain);
    goto out;
  }
  progress(f, p, p->toolchain, 1, 1);

out:

  return r;
}


int
fatso_guess_toolchain(struct fatso* f, struct fatso_package* p, struct fatso_toolchain* out_toolchain) {

  if( p->toolchain != NULL ){
    fatso_logf(f, FATSO_LOG_INFO, "Using explicitly provided toolchain '%s'.", p->toolchain );

    // TODO: optionally set this to p->toolchain
    out_toolchain->name = "[config provided]";
    out_toolchain->build = &build_provided_toolchain;
    out_toolchain->install = &install_provided_toolchain;
    return 0; // we found the toolchain
  }

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
