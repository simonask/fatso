#include "fatso.h"
#include "internal.h"

#include <string.h> // strerror
#include <errno.h>
#include <sys/param.h> // MAXPATHLEN

int
fatso_init_toolchain_plain_make(struct fatso_toolchain* toolchain) {
  toolchain->name = "make";
  toolchain->build = fatso_toolchain_run_make;
  toolchain->install = fatso_toolchain_run_make_install;
  return 0;
}

int
fatso_toolchain_run_make(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
  int r, r2;
  char* cmd = NULL;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);
  char* original_wd = fatso_alloc(MAXPATHLEN);
  char* makefile_path = NULL;
  original_wd = getcwd(original_wd, MAXPATHLEN);

  r = chdir(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "chdir: %s", strerror(errno));
    goto out;
  }

  asprintf(&makefile_path, "%s/Makefile", build_path);
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

int
fatso_toolchain_run_make_install(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
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
