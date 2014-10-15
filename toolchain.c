#include "fatso.h"
#include "internal.h"

#include <unistd.h> // getwd, chdir
#include <sys/param.h> // MAXPATHLEN
#include <string.h> // strerror
#include <errno.h>

static int
configure_and_make(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* callbacks) {
  int r, r2;
  char* cmd = NULL;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);
  char* original_wd = fatso_alloc(MAXPATHLEN);
  char* makefile_path = NULL;
  original_wd = getcwd(original_wd, MAXPATHLEN);

  // TODO: Append configuration.

  r = chdir(build_path);
  if (r != 0) {
    perror("chdir");
    goto out;
  }

  asprintf(&makefile_path, "%s/Makefile", build_path);

  if (!fatso_file_exists(makefile_path)) {
    asprintf(&cmd, "%s/configure --prefix=%s", build_path, install_prefix);
    progress(f, p, "configure", 0, 1);
    r = fatso_system_defer_output_until_error(cmd);
    if (r != 0) {
      fatso_logf(f, FATSO_LOG_FATAL, "Error during configure.");
      goto out_with_chdir;
    }
    progress(f, p, "configure", 1, 1);
    fatso_free(cmd);
  }

  asprintf(&cmd, "make -j%u -f %s", fatso_get_number_of_cpu_cores(), makefile_path);

  progress(f, p, cmd, 0, 1);
  r = fatso_system_defer_output_until_error(cmd);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during make.");
    goto out_with_chdir;
  }
  progress(f, p, cmd, 1, 1);

out_with_chdir:
  r2 = chdir(original_wd);
  if (r2 != 0) {
    r = r2;
    fatso_logf(f, FATSO_LOG_WARN, "chdir: %s", strerror(errno));
  }
out:
  fatso_free(makefile_path);
  fatso_free(original_wd);
  fatso_free(install_prefix);
  fatso_free(build_path);
  fatso_free(cmd);
  return r;
}

static int
make_install(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* callbacks) {
  int r, r2;
  char* original_wd = fatso_alloc(MAXPATHLEN);
  original_wd = getcwd(original_wd, MAXPATHLEN);
  char* build_path = fatso_package_build_path(f, p);
  r = chdir(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "chdir: %s", strerror(errno));
    goto out;
  }
  progress(f, p, "make install", 0, 1);
  r = fatso_system_defer_output_until_error("make install");
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during make install.");
    goto out_with_chdir;
  }
  progress(f, p, "make install", 1, 1);

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

  out_toolchain->name = "make";
  out_toolchain->build = configure_and_make;
  out_toolchain->install = make_install;
  return 0;
}
