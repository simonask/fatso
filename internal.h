#pragma once
#ifndef FATSO_INTERNAL_H_INCLUDED
#define FATSO_INTERNAL_H_INCLUDED

#include <stddef.h> // size_t
#include <stdio.h> // FILE*
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

struct fatso;
struct fatso_package;

void* fatso_alloc(size_t size);
void* fatso_calloc(size_t count, size_t size);
void* fatso_reallocf(void* ptr, size_t new_size);
void fatso_free(void* ptr);

void fatso_logz(struct fatso*, int level, const char* message, size_t length);
void fatso_logf(struct fatso*, int level, const char* fmt, ...);

#define BLACK   "\033[22;30m"
#define RED     "\033[01;31m"
#define GREEN   "\033[01;32m"
#define MAGENTA "\033[01;35m"
#define CYAN    "\033[01;36m"
#define YELLOW  "\033[01;33m"
#define RESET   "\033[00m"

struct yaml_node_s;
struct yaml_document_s;
struct yaml_parser_s;
struct yaml_node_s* fatso_yaml_mapping_lookup(struct yaml_document_s*, struct yaml_node_s*, const char* key);
struct yaml_node_s* fatso_yaml_sequence_lookup(struct yaml_document_s*, struct yaml_node_s*, size_t idx);
struct yaml_node_s* fatso_yaml_traverse(struct yaml_document_s*, struct yaml_node_s*, const char* keys[], size_t num_keys);
char* fatso_yaml_scalar_strdup(struct yaml_node_s*);
size_t fatso_yaml_sequence_length(struct yaml_node_s*);
size_t fatso_yaml_mapping_length(struct yaml_node_s*);

struct fatso_version {
  char* string;
  FATSO_ARRAY(char*) components;
};

void fatso_version_init(struct fatso_version*);
void fatso_version_copy(struct fatso_version* dst, const struct fatso_version* src);
void fatso_version_move(struct fatso_version* dst, struct fatso_version* src);
void fatso_version_destroy(struct fatso_version*);
void fatso_version_append_component(struct fatso_version*, const char* component, size_t n);
int fatso_version_from_string(struct fatso_version*, const char*);
int fatso_version_compare(const struct fatso_version* a, const struct fatso_version* b);
int fatso_version_compare_t(void* thunk, const void* a, const void* b);
int fatso_version_compare_r(const void* a, const void* b);
const char* fatso_version_string(const struct fatso_version*);

enum fatso_version_requirement {
  FATSO_VERSION_ANY,
  FATSO_VERSION_LT,
  FATSO_VERSION_LE,
  FATSO_VERSION_EQ,
  FATSO_VERSION_GT,
  FATSO_VERSION_GE,
  FATSO_VERSION_APPROXIMATELY
};

const char* fatso_version_requirement_to_string(enum fatso_version_requirement);

struct fatso_constraint {
  struct fatso_version version;
  enum fatso_version_requirement version_requirement;
};

int fatso_constraint_from_string(struct fatso_constraint*, const char*);
char* fatso_constraint_to_string(const struct fatso_constraint*);
const char* fatso_constraint_to_string_unsafe(const struct fatso_constraint*);
void fatso_constraint_destroy(struct fatso_constraint*);
bool fatso_version_matches_constraints(const struct fatso_version*, const struct fatso_constraint* constraints, size_t num_constraints);

struct fatso_dependency {
  char* name;
  FATSO_ARRAY(struct fatso_constraint) constraints;
};

void fatso_dependency_init(struct fatso_dependency*, const char* name, const struct fatso_constraint* constraints, size_t num_constraints);
void fatso_dependency_destroy(struct fatso_dependency*);
int fatso_dependency_parse(struct fatso_dependency*, struct yaml_document_s*, struct yaml_node_s*, char** out_error_message);
void fatso_dependency_add_constraint(struct fatso_dependency*, const struct fatso_constraint* constraint);

void fatso_kv_pair_init(struct fatso_kv_pair*, const char* key, const char* value);
void fatso_kv_pair_destroy(struct fatso_kv_pair*);

struct fatso_configuration {
  char* name;
  FATSO_ARRAY(struct fatso_dependency) dependencies;
  fatso_dictionary_t defines;
  fatso_dictionary_t env;
};

void fatso_configuration_init(struct fatso_configuration*);
void fatso_configuration_destroy(struct fatso_configuration*);
int fatso_configuration_parse(struct fatso_configuration* env, struct yaml_document_s*, struct yaml_node_s*, char** out_error_message);
void fatso_configuration_add_package(struct fatso* f, struct fatso_configuration* config, struct fatso_package* package);
void fatso_env_add_configuration(struct fatso* f, const struct fatso_configuration* config);
void fatso_env_add_package(struct fatso* f, struct fatso_package* p);

struct fatso_source_vtbl;

struct fatso_source {
  char* name;
  void* thunk;
  const struct fatso_source_vtbl* vtbl;
};

struct fatso_source_vtbl {
  const char* type;
  int(*fetch)(struct fatso*, struct fatso_package*, struct fatso_source*);
  int(*unpack)(struct fatso*, struct fatso_package*, struct fatso_source*);
  void(*free)(void* thunk);
};

int fatso_source_parse(struct fatso_source*, struct yaml_document_s*, struct yaml_node_s*, char** out_error_message);
void fatso_source_free(struct fatso_source*);
int fatso_source_fetch(struct fatso*, struct fatso_package*, struct fatso_source*);
int fatso_source_unpack(struct fatso*, struct fatso_package*, struct fatso_source*);

void fatso_tarball_source_init(struct fatso_source*, const char* url);
void fatso_git_source_init(struct fatso_source*, const char* url, const char* ref);

struct fatso_package_vtbl;

struct fatso_package {
  const struct fatso_package_vtbl* vtbl;
  char* name;
  struct fatso_version version;
  char* author;
  char* toolchain;
  struct fatso_source* source; // TODO: Multiple sources
  struct fatso_configuration base_configuration;
  FATSO_ARRAY(struct fatso_configuration) configurations;
};

struct fatso_package_vtbl {
  char*(*build_path)(struct fatso*, struct fatso_package*);
  char*(*install_prefix)(struct fatso*, struct fatso_package*);
};

void fatso_package_init(struct fatso_package*);
void fatso_package_destroy(struct fatso_package*);
int fatso_package_parse(struct fatso_package*, struct yaml_document_s*, struct yaml_node_s*, char** out_error_message);
int fatso_package_parse_from_file(struct fatso_package*, FILE* fp, char** out_error_message);
int fatso_package_parse_from_string(struct fatso_package*, const char* buffer, char** out_error_message);
char* fatso_package_build_path(struct fatso*, struct fatso_package*);
char* fatso_package_install_prefix(struct fatso*, struct fatso_package*);

typedef void(*fatso_report_progress_callback_t)(struct fatso*, void* identifier, const char* what, unsigned int progress, unsigned int total);

struct fatso_toolchain {
  const char* name;
  int(*build)(struct fatso*, struct fatso_package*, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* stdio_callbacks);
  int(*install)(struct fatso*, struct fatso_package*, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* stdio_callbacks);
};

int fatso_guess_toolchain(struct fatso*, struct fatso_package*, struct fatso_toolchain* out_chain);
int fatso_package_build_with_output(struct fatso* f, struct fatso_package* p, const struct fatso_toolchain* toolchain, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* stdio_callbacks);

// Toolchain initializers:
int fatso_init_toolchain_configure_and_make(struct fatso_toolchain* toolchain);
int fatso_init_toolchain_plain_make(struct fatso_toolchain*);
int fatso_init_toolchain_autotools_make(struct fatso_toolchain*);
int fatso_init_toolchain_cmake(struct fatso_toolchain*);
int fatso_init_toolchain_scons(struct fatso_toolchain*);
int fatso_init_toolchain_bjam(struct fatso_toolchain*);

// Toolchain guessers:
bool fatso_path_looks_like_configure_and_make_project(const char* path);
bool fatso_path_looks_like_plain_make_project(const char* path);
bool fatso_path_looks_like_autotools_project(const char* path);
bool fatso_path_looks_like_cmake_project(const char* path);
bool fatso_path_looks_like_scons_project(const char* path);
bool fatso_path_looks_like_bjam_project(const char* path);

// Toolchain common functions:
int fatso_toolchain_run_configure(struct fatso* f, struct fatso_package* package, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks);
int fatso_toolchain_run_make(struct fatso* f, struct fatso_package* package, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks);
int fatso_toolchain_run_make_install(struct fatso* f, struct fatso_package* package, fatso_report_progress_callback_t progress, const struct fatso_process_callbacks* io_callbacks);


enum fatso_repository_result {
  FATSO_PACKAGE_OK,
  FATSO_PACKAGE_UNKNOWN,
  FATSO_PACKAGE_NO_MATCHING_VERSION,
};

enum fatso_repository_result
fatso_repository_find_package_matching_dependency(struct fatso* f, struct fatso_dependency* dep, struct fatso_version* less_than_version, struct fatso_package** out_package);

enum fatso_repository_result
fatso_repository_find_package(struct fatso* f, const char* name, struct fatso_version* less_than_version, struct fatso_package** out_package);

struct fatso_project {
  struct fatso_package package; // must be first :)
  char* path;
  FATSO_ARRAY(struct fatso_package*) install_order;
};

void fatso_project_init(struct fatso_project*);
void fatso_project_destroy(struct fatso_project*);

enum fatso_dependency_graph_resolution_status {
  FATSO_DEPENDENCY_GRAPH_SUCCESS,
  FATSO_DEPENDENCY_GRAPH_CONFLICT,
  FATSO_DEPENDENCY_GRAPH_UNKNOWN,
};

struct fatso_dependency_package_pair {
  struct fatso_package* package;
  struct fatso_dependency* dependency;
};
typedef FATSO_ARRAY(struct fatso_dependency_package_pair) fatso_conflicts_t;
typedef FATSO_ARRAY(struct fatso_dependency*) fatso_unknown_dependencies_t;

struct fatso_dependency_graph;
struct fatso_dependency_graph* fatso_dependency_graph_for_package(struct fatso* f, struct fatso_package*, enum fatso_dependency_graph_resolution_status* out_status);
struct fatso_dependency_graph* fatso_dependency_graph_new();
struct fatso_dependency_graph* fatso_dependency_graph_copy(struct fatso_dependency_graph*);
void fatso_dependency_graph_free(struct fatso_dependency_graph*);
int fatso_dependency_graph_add_closed_set(struct fatso_dependency_graph*, struct fatso_package*);
int fatso_dependency_graph_add_open_set(struct fatso_dependency_graph*, struct fatso_dependency*, struct fatso_package* dependency_of);
void fatso_dependency_graph_topological_sort(struct fatso_dependency_graph*, struct fatso*, struct fatso_package***, size_t*);

void fatso_dependency_graph_get_conflicts(struct fatso_dependency_graph*, fatso_conflicts_t* out_conflicts);
void fatso_dependency_graph_get_unknown_dependencies(struct fatso_dependency_graph*, fatso_unknown_dependencies_t* out_deps);

#ifdef __cplusplus
}
#endif

#endif // FATSO_INTERNAL_H_INCLUDED
