#include "fatso.h"
#include "internal.h"

#include <string.h> // strerror
#include <errno.h>

bool
fatso_path_looks_like_scons_project(const char* path) {
  char* sconstruct_path;
  char* sconscript_path;
  asprintf(&sconstruct_path, "%s/SConstruct", path);
  asprintf(&sconscript_path, "%s/SConscript", path);
  bool r = fatso_file_exists(sconstruct_path) || fatso_file_exists(sconscript_path);
  fatso_free(sconstruct_path);
  fatso_free(sconscript_path);
  return r;
}

static int
scons_build(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
  int r;
  char* cmd = NULL;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);

  r = chdir(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "chdir: %s", strerror(errno));
    goto out;
  }

  asprintf(&cmd, "scons -j%u -Q PREFIX=%s", fatso_get_number_of_cpu_cores(), install_prefix);
  progress(f, p, cmd, 0, 1);
  r = fatso_system_defer_output_until_error(cmd);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during scons.");
  }
  progress(f, p, cmd, 1, 1);

out:
  fatso_free(cmd);
  fatso_free(build_path);
  fatso_free(install_prefix);
  return r;
}

static int
scons_install(struct fatso* f, struct fatso_package* p, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks) {
  int r;
  char* cmd = NULL;
  char* build_path = fatso_package_build_path(f, p);
  char* install_prefix = fatso_package_install_prefix(f, p);

  r = chdir(build_path);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "chdir: %s", strerror(errno));
    goto out;
  }

  asprintf(&cmd, "scons install -Q PREFIX=%s", install_prefix);
  progress(f, p, cmd, 0, 1);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Error during scons install.");
  }
  progress(f, p, cmd, 1, 1);

out:
  fatso_free(cmd);
  fatso_free(build_path);
  fatso_free(install_prefix);
  return r;
}

int
fatso_init_toolchain_scons(struct fatso_toolchain* toolchain) {
  toolchain->name = "scons";
  toolchain->build = scons_build;
  toolchain->install = scons_install;
  return 0;
}
