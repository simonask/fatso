#include "fatso.h"
#include "internal.h"
#include "util.h"
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

  struct {
    size_t size;
    char** data;
  } filtered_args = {0, NULL};

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
        fatso_set_project_directory(&fatso, optarg);
        break;
      default: {
        char* append = strdup(argv[optind]);
        fatso_push_back_v(&filtered_args, &append);
        break;
      }
    }
  }
  while (optind < argc) {
    char* append = strdup(argv[optind++]);
    fatso_push_back_v(&filtered_args, &append);
  }

  argc = filtered_args.size;
  argv = filtered_args.data;

  if (argc == 0) {
    return fatso_help(&fatso, 0, NULL);
  }

  static const named_command_t named_commands[] = {
    {"install", fatso_install},
    {"upgrade", fatso_upgrade},
    {"env", fatso_env},
    {"exec", fatso_exec},
    {"sync", fatso_sync},
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
