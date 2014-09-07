#include "fatso.h"

#include <stdio.h>
#include <string.h>

static void
usage(const char* program_name) {
  fprintf(stderr, "Usage:\n\t%s <command> [options]\n\n", program_name);
  fprintf(stderr,
    "Global options:"
    "\n\t-C <path>    Run Fatso with <path> as the working dir (default: .)"
    "\n\t-H <path>    Run Fatso with <path> as the Fatso home dir (default: $HOME/.fatso)"
    "\n\n");
  fprintf(stderr,
    "Commands:"
    "\n\tinit         Create an empty fatso.yml in the working dir, if none exists."
    "\n\tbuild        Builds the current Fatso project."
    "\n\tinstall      Installs dependencies for the current Fatso project."
    "\n\tupgrade      Upgrades dependencies for the current Fatso project. Implies 'sync'."
    "\n\tsync         Get latest package descriptions from online repository."
    "\n\tclean        Clean slate."
    "\n\tinfo         Displays info about a package."
    "\n\thelp         Displays this help text."
    "\n\tcflags       Outputs the necessary CFLAGS to build with project dependencies."
    "\n\tldflags      Outputs the necessary LDFLAGS to link against project dependencies."
    "\n\n"
    "To get help about an individual command, run `%s help <command>`.\n", program_name);
}

static void
init_usage(const char* program_name) {}

static void
build_usage(const char* program_name) {}

static void
install_usage(const char* program_name) {}

static void
upgrade_usage(const char* program_name) {}

static void
sync_usage(const char* program_name) {}

static void
info_usage(const char* program_name) {}

static void
cflags_usage(const char* program_name) {}

static void
ldflags_usage(const char* program_name) {}

struct command_usage {
  const char* name;
  void(*display_usage)(const char*);
};

int
fatso_help(struct fatso* f, int argc, char* const* argv) {

  static const struct command_usage cu[] = {
    {"init", init_usage},
    {"build", build_usage},
    {"install", install_usage},
    {"upgrade", upgrade_usage},
    {"sync", sync_usage},
    {"info", info_usage},
    {"cflags", cflags_usage},
    {"ldflags", ldflags_usage},
    {NULL, NULL}
  };

  if (argc > 1) {
    for (const struct command_usage* p = cu; p->name; ++p) {
      if (strcmp(p->name, argv[1]) == 0) {
        p->display_usage(f->program_name);
        return 1;
      }
    }
  }

  usage(f->program_name);
  return 1;
}
