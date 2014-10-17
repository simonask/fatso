#include "fatso.h"
#include "internal.h"

#include <string.h> // strerror
#include <errno.h>
#include <sys/param.h> // MAXPATHLEN
#include <unistd.h> // getwd, chdir

bool
fatso_path_looks_like_configure_and_make_project(const char* path) {
  char* configure_path;
  asprintf(&configure_path, "%s/configure", path);
  bool r = fatso_file_exists(configure_path);
  fatso_free(configure_path);
  return r;
}

bool
fatso_path_looks_like_autotools_project(const char* path) {
  char* autogen_sh_path;
  asprintf(&autogen_sh_path, "%s/autogen.sh", path);
  bool r = fatso_file_exists(autogen_sh_path);
  fatso_free(autogen_sh_path);
  return r;
}

int
fatso_toolchain_run_configure(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
  int r;
  char* cmd = NULL;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);
  char* makefile_path = NULL;

  r = chdir(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "chdir: %s", strerror(errno));
    goto out;
  }

  asprintf(&makefile_path, "%s/Makefile", build_path);

  if (!fatso_file_exists(makefile_path)) {
    asprintf(&cmd, "%s/configure --prefix=%s", build_path, install_prefix);
    progress(f, p, "configure", 0, 1);
    r = fatso_system_defer_output_until_error(cmd);
    if (r != 0) {
      fatso_logf(f, FATSO_LOG_FATAL, "Error during configure.");
      goto out;
    }
    progress(f, p, "configure", 1, 1);
  }

out:
  fatso_free(cmd);
  fatso_free(makefile_path);
  fatso_free(build_path);
  fatso_free(install_prefix);
  return r;
}

int
fatso_toolchain_run_configure_and_make(struct fatso* f, struct fatso_package* package, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
  int r = fatso_toolchain_run_configure(f, package, progress, io_callbacks);
  if (r == 0) {
    r = fatso_toolchain_run_make(f, package, progress, io_callbacks);
  }
  return r;
}

int
fatso_init_toolchain_configure_and_make(struct fatso_toolchain* toolchain) {
  toolchain->name = "configure_and_make";
  toolchain->build = fatso_toolchain_run_configure_and_make;
  toolchain->install = fatso_toolchain_run_make_install;
  return 0;
}

int
fatso_init_toolchain_autotools_make(struct fatso_toolchain* toolchain) {
  // TODO: Handle autogen.sh?
  return fatso_init_toolchain_configure_and_make(toolchain);
}
