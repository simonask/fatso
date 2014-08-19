#include "fatso.h"
#include <string.h>

typedef struct named_comand {
  const char* name;
  fatso_command_t command;
} named_command_t;

int
main(int argc, char const* argv[])
{
  struct fatso fatso;
  int r;

  fatso.program_name = argv[0];

  if ((r = fatso_init(&fatso)) != 0) {
    return r;
  }

  if (argc < 2) {
    return fatso_help(&fatso, argc, argv);
  }

  static const named_command_t named_commands[] = {
    {"update", fatso_update},
    {"help", fatso_help},
    {"--help", fatso_help},
    {"-h", fatso_help},
    {NULL, NULL}
  };

  for (const named_command_t* p = named_commands; p->name != NULL; ++p) {
    if (strcmp(p->name, argv[1]) == 0) {
      fatso.command = p->command;
      break;
    }
  }

  if (fatso.command == NULL) {
    fatso.command = fatso_help;
  }

  return fatso.command(&fatso, argc - 1, argv + 1);
}
