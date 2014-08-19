#include "fatso.h"

#include <stdio.h>

static void
usage(const char* program_name) {
  fprintf(stderr, "Usage:\n\t%s <command> [options]\n\n", program_name);
  fprintf(stderr, "Commands:\n\tbuild\n\tinstall\n\tupgrade\n\tinfo\n\thelp\n\tcflags\n\tldflags\n");
}

int
fatso_help(struct fatso* f, int argc, char const* argv[]) {
  if (argc == 1) {
    usage(f->program_name);
  }
  return 1;
}
