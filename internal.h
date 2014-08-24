#pragma once
#ifndef FATSO_INTERNAL_H_INCLUDED
#define FATSO_INTERNAL_H_INCLUDED

#include <stddef.h> // size_t
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

void* fatso_alloc(size_t size);
void* fatso_calloc(size_t count, size_t size);
void* fatso_reallocf(void* ptr, size_t new_size);
void fatso_free(void* ptr);

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
void fatso_constraint_destroy(struct fatso_constraint*);

struct fatso_dependency {
  char* name;
  FATSO_ARRAY(struct fatso_constraint) constraints;
};

void fatso_dependency_init(struct fatso_dependency*, const char* name, const struct fatso_constraint* constraints, size_t num_constraints);
void fatso_dependency_destroy(struct fatso_dependency*);

struct fatso_define {
  char* key;
  char* value;
};

void fatso_define_init(struct fatso_define*, const char* key, const char* value);
void fatso_define_destroy(struct fatso_define*);

struct fatso_environment {
  char* name;
  FATSO_ARRAY(struct fatso_dependency) dependencies;
  FATSO_ARRAY(struct fatso_define) defines;
};

void fatso_environment_init(struct fatso_environment*);
void fatso_environment_destroy(struct fatso_environment*);

struct fatso_package {
  char* name;
  struct fatso_version version;
  char* author;
  char* toolchain; // TODO: Autodetect
  struct fatso_environment base_environment;
  FATSO_ARRAY(struct fatso_environment) environments;
};

void fatso_package_init(struct fatso_package*);
void fatso_package_destroy(struct fatso_package*);

struct fatso_project {
  char* path;
  struct fatso_package package;
  FATSO_ARRAY(struct fatso_package*) install_order;
};

void fatso_project_init(struct fatso_project*);
void fatso_project_destroy(struct fatso_project*);

enum fatso_dependency_graph_resolution_status {
  FATSO_DEPENDENCY_GRAPH_SUCCESS,
  FATSO_DEPENDENCY_GRAPH_CONFLICT,
  FATSO_DEPENDENCY_GRAPH_CIRCULAR,
};

struct fatso_dependency_graph;
struct fatso_dependency_graph* fatso_dependency_graph_new();
struct fatso_dependency_graph* fatso_dependency_graph_copy(struct fatso_dependency_graph*);
void fatso_dependency_graph_free(struct fatso_dependency_graph*);
void fatso_dependency_graph_add_package(struct fatso_dependency_graph*, struct fatso_package*);
void fatso_dependency_graph_add_dependency(struct fatso_dependency_graph*, struct fatso_dependency*);
enum fatso_dependency_graph_resolution_status fatso_dependency_graph_resolve(struct fatso_dependency_graph*);
void fatso_dependency_graph_topological_sort(struct fatso_dependency_graph*, struct fatso_package***, size_t*);

struct yaml_node_s;
struct yaml_document_s;
struct yaml_parser_s;
struct yaml_node_s* fatso_yaml_mapping_lookup(struct yaml_document_s*, struct yaml_node_s*, const char* key);
struct yaml_node_s* fatso_yaml_sequence_lookup(struct yaml_document_s*, struct yaml_node_s*, size_t idx);
struct yaml_node_s* fatso_yaml_traverse(struct yaml_document_s*, struct yaml_node_s*, const char* keys[], size_t num_keys);
char* fatso_yaml_scalar_strdup(struct yaml_node_s*);
size_t fatso_yaml_sequence_length(struct yaml_node_s*);
size_t fatso_yaml_mapping_length(struct yaml_node_s*);

#ifdef __cplusplus
}
#endif

#endif // FATSO_INTERNAL_H_INCLUDED
