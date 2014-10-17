#include "fatso.h"
#include "internal.h"

int
fatso_guess_toolchain(struct fatso* f, struct fatso_package* p, struct fatso_toolchain* out_toolchain) {
  // TODO: Use 'toolchain' option in package.
  // TODO: Auto-detect CMakeLists.txt, bjam, SConstruct, etc.
  return fatso_init_toolchain_configure_and_make(out_toolchain);
}
