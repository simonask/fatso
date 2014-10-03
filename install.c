#include "fatso.h"
#include "internal.h"
#include "util.h"

#include <stdio.h>
#include <stdarg.h>

int
fatso_install(struct fatso* f, int argc, char* const* argv) {
  int r = 0;

  r = fatso_load_project(f);
  if (r != 0) goto out;

  char* packages_git_dir;
  asprintf(&packages_git_dir, "%s/packages/.git", fatso_home_directory(f));
  if (!fatso_directory_exists(packages_git_dir)) {
    fatso_logf(f, FATSO_LOG_FATAL, "Repository is empty. Please run `fatso sync`.");
    r = 1;
    goto out;
  }
  fatso_free(packages_git_dir);

  r = fatso_load_or_generate_dependency_graph(f);
  if (r != 0) goto out;

  r = fatso_install_dependencies(f);
  if (r != 0) goto out;

out:
  return r;
}

enum install_status {
  INSTALL_STATUS_OK,
  INSTALL_STATUS_WORKING,
  INSTALL_STATUS_ERROR,
};

static void
print_progress(struct fatso* f, struct fatso_package* p, enum install_status is, const char* fmt, ...) {
  const char* color;
  switch (is) {
    case INSTALL_STATUS_OK: color = GREEN; break;
    case INSTALL_STATUS_WORKING: color = YELLOW; break;
    case INSTALL_STATUS_ERROR: color = RED; break;
  }
  const char* version = fatso_version_string(&p->version);
  printf("\33[2K\r%s%s %s" RESET ": ", color, p->name, version);
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

int
fatso_package_download(struct fatso* f, struct fatso_package* p, struct fatso_source** out_chosen_source) {
  if (p->source == NULL) {
    print_progress(f, p, INSTALL_STATUS_ERROR, "Invalid package definition, no source specified!");
    return 1;
  } else {
    *out_chosen_source = p->source;
    return fatso_source_fetch(f, p, p->source);
  }
}

int
fatso_package_unpack(struct fatso* f, struct fatso_package* p, struct fatso_source* source) {
  return fatso_source_unpack(f, p, source);
}

int
fatso_package_download_and_unpack(struct fatso* f, struct fatso_package* p) {
  struct fatso_source* chosen_source = NULL;
  int r;

  print_progress(f, p, INSTALL_STATUS_WORKING, "Downloading...");

  r = fatso_package_download(f, p, &chosen_source);
  if (r != 0)
    goto out;

  print_progress(f, p, INSTALL_STATUS_WORKING, "Unpacking...");

  r = fatso_package_unpack(f, p, chosen_source);
  if (r != 0)
    goto out;

out:
  return r;
}

static void
print_build_progress(struct fatso* f, void* userdata, const char* what, unsigned int progress, unsigned int total) {
  print_progress(f, userdata, progress < total ? INSTALL_STATUS_WORKING : INSTALL_STATUS_OK, "%s (%u/%u)", what, progress, total);
}

static void
print_install_progress(struct fatso* f, void* userdata, const char* what, unsigned int progress, unsigned int total) {
  print_progress(f, userdata, progress < total ? INSTALL_STATUS_WORKING : INSTALL_STATUS_OK, "%s (%u/%u)", what, progress, total);
}

int
fatso_package_build(struct fatso* f, struct fatso_package* p, const struct fatso_toolchain* toolchain) {
  print_progress(f, p, INSTALL_STATUS_WORKING, "Building...");
  return toolchain->build(f, p, print_build_progress);
}

int
fatso_package_install_products(struct fatso* f, struct fatso_package* p, const struct fatso_toolchain* toolchain) {
  print_progress(f, p, INSTALL_STATUS_WORKING, "Installing...");
  return toolchain->install(f, p, print_install_progress);
}

int
fatso_package_install(struct fatso* f, struct fatso_package* p) {
  int r;

  r = fatso_package_download_and_unpack(f, p);
  if (r != 0)
    goto out;

  struct fatso_toolchain toolchain;
  r = fatso_guess_toolchain(f, p, &toolchain);
  if (r != 0) {
    print_progress(f, p, INSTALL_STATUS_ERROR, "Error guessing toolchain.\n");
    goto out;
  }

  r = fatso_package_build(f, p, &toolchain);
  if (r != 0)
    return r;

  r = fatso_package_install_products(f, p, &toolchain);
  if (r != 0)
    return r;

  // r = fatso_package_append_build_flags(f, p);
  // if (r != 0)
  //   return r;
out:
  printf("\n");
  return r;
}

int
fatso_install_dependencies(struct fatso* f) {
  for (size_t i = 0; i < f->project->install_order.size; ++i) {
    struct fatso_package* p = f->project->install_order.data[i];
    int r = fatso_package_install(f, p);
    if (r != 0)
      return r;
  }
  return 0;
}
