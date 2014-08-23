#pragma once
#ifndef FATSO_H_INCLUDED
#define FATSO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct fatso;
struct fatso_project;

typedef int(*fatso_command_t)(struct fatso*, int argc, char* const* argv);

struct fatso {
  const char* program_name;
  fatso_command_t command;
  char* global_dir;
  char* working_dir;
  struct fatso_project* project;
};

int fatso_init(struct fatso*, const char* program_name);
void fatso_destroy(struct fatso*);
void fatso_set_home_directory(struct fatso*, const char*);
void fatso_set_working_directory(struct fatso*, const char*);

// Repository functions:
int fatso_update_packages(struct fatso*);

// Project functions:
int fatso_load_project(struct fatso*);
void fatso_unload_project(struct fatso*);
int fatso_load_dependency_graph(struct fatso*);
int fatso_generate_dependency_graph(struct fatso*);
int fatso_load_or_generate_dependency_graph(struct fatso*);
int fatso_install_dependencies(struct fatso*);


const char* fatso_project_path(struct fatso*);

// Commands:
int fatso_install(struct fatso*, int argc, char* const* argv);
int fatso_update(struct fatso*, int argc, char* const* argv);
int fatso_help(struct fatso*, int argc, char* const* argv);
int fatso_info(struct fatso*, int argc, char* const* argv);

#ifdef __cplusplus
}
#endif

#endif // FATSO_H_INCLUDED
