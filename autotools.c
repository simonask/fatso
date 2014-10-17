#include "fatso.h"
#include "internal.h"

#include <string.h> // strerror
#include <errno.h>
#include <sys/param.h> // MAXPATHLEN
#include <unistd.h> // getwd, chdir

int
fatso_toolchain_run_configure(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
  int r, r2;
  r = 0;
  char* cmd = NULL;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);
  char* makefile_path = NULL;
  char* original_wd = fatso_alloc(MAXPATHLEN);
  original_wd = getcwd(original_wd, MAXPATHLEN);

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
      goto out_with_chdir;
    }
    progress(f, p, "configure", 1, 1);
    fatso_free(cmd);
  }

out_with_chdir:
  r2 = chdir(original_wd);
  if (r2 != 0) {
    r = r2;
    fatso_logf(f, FATSO_LOG_WARN, "chdir: %s", strerror(errno));
  }
out:
  fatso_free(makefile_path);
  fatso_free(build_path);
  fatso_free(original_wd);
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
