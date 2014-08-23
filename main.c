#include "fatso.h"
#include "internal.h"
#include <string.h>
#include <getopt.h>
#include <stdio.h>

typedef struct named_comand {
  const char* name;
  fatso_command_t command;
} named_command_t;

int
main(int argc, char* const* argv)
{
  struct fatso fatso;
  int r;

  if ((r = fatso_init(&fatso, argv[0])) != 0) {
    return r;
  }

  int filtered_argc = 0;
  char** filtered_argv = NULL;

  int c;
  while (1) {
    static struct option long_options[] = {
      {"home", required_argument, NULL, 'H'},
      {"work", required_argument, NULL, 'C'},
      {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "H:C:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
      case 'H':
        fatso_set_home_directory(&fatso, optarg);
        break;
      case 'C':
        fatso_set_working_directory(&fatso, optarg);
        break;
      default:
        filtered_argv = fatso_reallocf(filtered_argv, sizeof(char*) * (filtered_argc + 1));
        filtered_argv[filtered_argc++] = strdup(argv[optind]);
        break;
    }
  }
  while (optind < argc) {
    filtered_argv = fatso_reallocf(filtered_argv, sizeof(char*) * (filtered_argc + 1));
    filtered_argv[filtered_argc++] = strdup(argv[optind]);
    ++optind;
  }

  argc = filtered_argc;
  argv = filtered_argv;

  if (argc == 0) {
    return fatso_help(&fatso, 0, NULL);
  }

  static const named_command_t named_commands[] = {
    {"install", fatso_install},
    {"update", fatso_update},
    {"info", fatso_info},
    {"help", fatso_help},
    {"--help", fatso_help},
    {"-h", fatso_help},
    {NULL, NULL}
  };

  for (const named_command_t* p = named_commands; p->name != NULL; ++p) {
    if (strcmp(p->name, argv[0]) == 0) {
      fatso.command = p->command;
      break;
    }
  }

  if (fatso.command == NULL) {
    fatso.command = fatso_help;
  }

  return fatso.command(&fatso, argc, argv);
}
