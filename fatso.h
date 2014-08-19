#pragma once
#ifndef FATSO_H_INCLUDED
#define FATSO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct fatso;

typedef int(*fatso_command_t)(struct fatso*, int argc, char const* argv[]);

struct fatso {
  const char* program_name;
  fatso_command_t command;
  char* global_dir;
  char* local_dir;
};

int fatso_init(struct fatso*);
void fatso_destroy(struct fatso*);

// Commands:
int fatso_update(struct fatso*, int argc, char const* argv[]);
int fatso_help(struct fatso*, int argc, char const* argv[]);

#ifdef __cplusplus
}
#endif

#endif // FATSO_H_INCLUDED
