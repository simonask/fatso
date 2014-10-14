#pragma once
#ifndef FATSO_H_INCLUDED
#define FATSO_H_INCLUDED

#include <stddef.h> // size_t

#ifdef __cplusplus
extern "C" {
#endif

struct fatso;
struct fatso_project;
struct fatso_logger;
struct fatso_configuration;

typedef int(*fatso_command_t)(struct fatso*, int argc, char* const* argv);

struct fatso {
  const char* program_name;
  fatso_command_t command;
  char* global_dir;
  char* working_dir;
  struct fatso_project* project;
  const struct fatso_logger* logger;
  struct fatso_configuration* consolidated_configuration;
};

enum fatso_log_level {
  FATSO_LOG_INFO,
  FATSO_LOG_WARN,
  FATSO_LOG_FATAL,
};

struct fatso_logger {
  void(*log)(struct fatso*, int level, const char* message, size_t length);
};

int fatso_init(struct fatso*, const char* program_name);
void fatso_destroy(struct fatso*);
void fatso_set_logger(struct fatso*, const struct fatso_logger*);
void fatso_set_home_directory(struct fatso*, const char*);
void fatso_set_project_directory(struct fatso*, const char*);
const char* fatso_home_directory(struct fatso*);
const char* fatso_project_directory(struct fatso*);
const char* fatso_packages_directory(struct fatso*);
int fatso_load_environment(struct fatso*);

// Repository functions:
int fatso_sync_packages(struct fatso*);

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
int fatso_build(struct fatso*, int argc, char* const* argv);
int fatso_upgrade(struct fatso*, int argc, char* const* argv);
int fatso_sync(struct fatso*, int argc, char* const* argv);
int fatso_env(struct fatso*, int argc, char* const* argv);
int fatso_exec(struct fatso*, int argc, char* const* argv);
int fatso_help(struct fatso*, int argc, char* const* argv);
int fatso_info(struct fatso*, int argc, char* const* argv);

#ifdef __cplusplus
}
#endif

#endif // FATSO_H_INCLUDED
