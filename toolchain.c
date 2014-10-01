#include "fatso.h"
#include "internal.h"

#include <unistd.h> // getwd, chdir
#include <sys/param.h> // MAXPATHLEN
#include <string.h> // strerror
#include <errno.h>

static void ignore_output(struct fatso_process* p, const void* buffer, size_t len) {}
static void forward_stderr(struct fatso_process* p, const void* buffer, size_t len) {
  write(fileno(stderr), buffer, len);
}

static const struct fatso_process_callbacks g_forward_stderr = {
  .on_stdout = ignore_output,
  .on_stderr = forward_stderr,
};

static int
configure_and_make(struct fatso* f, struct fatso_package* p) {
  int r, r2;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);
  char* original_wd = fatso_alloc(MAXPATHLEN);
  original_wd = getcwd(original_wd, MAXPATHLEN);

  // TODO: Append configuration.

  char* cmd;
  asprintf(&cmd, "%s/configure --prefix=%s", build_path, install_prefix);
  r = chdir(build_path);
  if (r != 0) {
    perror("chdir");
    goto out;
  }

  r = fatso_system_with_callbacks(cmd, &g_forward_stderr);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during configure.");
    goto out_with_chdir;
  }

  r = fatso_system_with_callbacks("make", &g_forward_stderr);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during make.");
    goto out_with_chdir;
  }

out_with_chdir:
  r2 = chdir(original_wd);
  if (r2 != 0) {
    r = r2;
    fatso_logf(f, FATSO_LOG_WARN, "chdir: %s", strerror(errno));
  }
out:
  fatso_free(original_wd);
  fatso_free(install_prefix);
  fatso_free(build_path);
  return r;
}

static int
make_install(struct fatso* f, struct fatso_package* p) {
  int r, r2;
  char* original_wd = fatso_alloc(MAXPATHLEN);
  original_wd = getcwd(original_wd, MAXPATHLEN);
  char* build_path = fatso_package_build_path(f, p);
  r = chdir(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "chdir: %s", strerror(errno));
    goto out;
  }
  r = fatso_system_with_callbacks("make install", &g_forward_stderr);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during make install.");
    goto out_with_chdir;
  }

out_with_chdir:
  r2 = chdir(original_wd);
  if (r2 != 0) {
    r = r2;
    fatso_logf(f, FATSO_LOG_WARN, "chdir: %s", strerror(errno));
  }
out:
  fatso_free(build_path);
  fatso_free(original_wd);
  return r;
}

int
fatso_guess_toolchain(struct fatso* f, struct fatso_package* p, struct fatso_toolchain* out_toolchain) {
  // TODO: Use 'toolchain' option in package.
  // TODO: Auto-detect CMakeLists.txt, bjam, SConstruct, etc.

  out_toolchain->build = configure_and_make;
  out_toolchain->install = make_install;
  return 0;
}
