#include "fatso.h"
#include "internal.h"

#include <stdio.h> // write

static void
ignore_progress(struct fatso* f, void* userdata, const char* what, unsigned int progress, unsigned int total) {
}

static void
forward_stdout(struct fatso_process* p, const void* buffer, size_t len) {
  write(fileno(stdout), buffer, len);
}

static void
forward_stderr(struct fatso_process* p, const void* buffer, size_t len) {
  write(fileno(stderr), buffer, len);
}

int
fatso_build(struct fatso* f, int argc, char* const* argv) {
  int r;

  r = fatso_load_project(f);
  if (r != 0)
    goto out;

  r = fatso_load_or_generate_dependency_graph(f);
  if (r != 0)
    goto out;

  r = fatso_load_environment(f);
  if (r != 0)
    goto out;

  struct fatso_toolchain toolchain;
  r = fatso_guess_toolchain(f, &f->project->package, &toolchain);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Could not guess toolchain! Please specify in fatso.yml. :-)");
    goto out;
  }
  fatso_logf(f, FATSO_LOG_INFO, "Building '%s' with '%s'...", f->project->package.name, toolchain.name);

  const struct fatso_process_callbacks callbacks = {
    .on_stdout = forward_stdout,
    .on_stderr = forward_stderr,
  };

  r = fatso_package_build_with_output(f, &f->project->package, &toolchain, ignore_progress, &callbacks);
  if (r != 0) {
    goto out;
  }

out:
  return r;
}
