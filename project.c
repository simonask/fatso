#include "fatso.h"
#include "util.h"
#include "internal.h"

#include <yaml.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h> // isspace

void fatso_project_init(struct fatso_project* p) {
  memset(p, 0, sizeof(*p));
}

void
fatso_project_destroy(struct fatso_project* p) {
  fatso_free(p->path);
  fatso_free(p->name);
  fatso_version_destroy(&p->version);
  fatso_free(p->author);
  fatso_free(p->toolchain);
  fatso_environment_destroy(&p->base_environment);
  for (size_t i = 0; i < p->environments.size; ++i) {
    fatso_environment_destroy(&p->environments.data[i]);
  }
  fatso_free(p->environments.data);
  memset(p, 0, sizeof(*p));
}

void
fatso_unload_project(struct fatso* f) {
  fatso_project_destroy(f->project);
  fatso_free(f->project);
  f->project = NULL;
}

static char*
find_fatso_yml(const char* working_dir) {
  char* r = strdup(working_dir);
  char* check;
  asprintf(&check, "%s/fatso.yml", r);

  while (!fatso_file_exists(check)) {
    char* p = strrchr(r, '/');
    if (p == r || p == NULL) {
      goto not_found;
    }
    *p = '\0';
    sprintf(check, "%s/fatso.yml", r);
  }
  fatso_free(check);
  return r;
not_found:
  fatso_free(r);
  fatso_free(check);
  return NULL;
}

static int
parse_fatso_dependency(struct fatso_dependency* dep, yaml_document_t* doc, yaml_node_t* node, char** out_error_message) {
  int r = 1;
  char* version_requirement_string = NULL;

  if (node->type != YAML_SEQUENCE_NODE || fatso_yaml_sequence_length(node) == 0) {
    *out_error_message = strdup("Invalid dependency, expected a tuple of [package, version].");
    goto out;
  }

  yaml_node_t* package_name = fatso_yaml_sequence_lookup(doc, node, 0);

  yaml_node_t* version_or_options = fatso_yaml_sequence_lookup(doc, node, 1);
  struct fatso_constraint constraint = {{0}}; // TODO: Support >1 constraint
  if (version_or_options == NULL) {
    fatso_version_from_string(&constraint.version, "");
    constraint.version_requirement = FATSO_VERSION_ANY;
  } else if (version_or_options->type == YAML_SCALAR_NODE) {
    version_requirement_string = fatso_yaml_scalar_strdup(version_or_options);
    struct fatso_constraint constraint;
    r = fatso_constraint_from_string(&constraint, version_requirement_string);
    if (r != 0)
      goto error;
  } else {
    *out_error_message = strdup("Invalid version/options!");
    goto out;
  }

  char* name = fatso_yaml_scalar_strdup(package_name);
  fatso_dependency_init(dep, name, &constraint, 1);
  fatso_free(name);
out:
  fatso_free(version_requirement_string);
  return r;
error:
  fatso_dependency_destroy(dep);
  goto out;
}

static int
parse_fatso_environment(struct fatso_environment* e, yaml_document_t* doc, yaml_node_t* node, char** out_error_message) {
  int r = 0;

  yaml_node_t* dependencies = fatso_yaml_mapping_lookup(doc, node, "dependencies");
  if (dependencies && dependencies->type == YAML_SEQUENCE_NODE) {
    size_t len = fatso_yaml_sequence_length(dependencies);
    if (len != 0) {
      e->dependencies.size = len;
      e->dependencies.data = fatso_calloc(len, sizeof(struct fatso_dependency));
      for (size_t i = 0; i < len; ++i) {
        r = parse_fatso_dependency(&e->dependencies.data[i], doc, fatso_yaml_sequence_lookup(doc, dependencies, i), out_error_message);
        if (r != 0)
          goto out;
      }
    }
  }

  yaml_node_t* defines = fatso_yaml_mapping_lookup(doc, node, "defines");
  if (defines && defines->type == YAML_MAPPING_NODE) {
    size_t len = fatso_yaml_mapping_length(defines);
    if (len != 0) {
      e->defines.size = len;
      e->defines.data = fatso_calloc(len, sizeof(struct fatso_define));
      for (size_t i = 0; i < len; ++i) {
        yaml_node_pair_t* pair = &defines->data.mapping.pairs.start[i];
        yaml_node_t* key = yaml_document_get_node(doc, pair->key);
        yaml_node_t* value = yaml_document_get_node(doc, pair->value);
        e->defines.data[i].key = fatso_yaml_scalar_strdup(key);
        e->defines.data[i].value = fatso_yaml_scalar_strdup(value);
      }
    }
  }

out:
  return r;
}

static int
parse_fatso_project(struct fatso_project* p, yaml_document_t* doc, char** out_error_message) {
  yaml_node_t* root = yaml_document_get_root_node(doc);
  if (root == NULL) {
    *out_error_message = strdup("YAML file does not contain a document");
    return 1;
  }

  yaml_node_t* name_node = fatso_yaml_mapping_lookup(doc, root, "project");
  if (name_node) {
    p->name = fatso_yaml_scalar_strdup(name_node);
  }

  yaml_node_t* version_node = fatso_yaml_mapping_lookup(doc, root, "version");
  if (version_node) {
    char* version = fatso_yaml_scalar_strdup(version_node);
    fatso_version_from_string(&p->version, version);
    fatso_free(version);
  }

  yaml_node_t* author_node = fatso_yaml_mapping_lookup(doc, root, "author");
  if (author_node) {
    p->author = fatso_yaml_scalar_strdup(author_node);
  }

  yaml_node_t* toolchain_node = fatso_yaml_mapping_lookup(doc, root, "toolchain");
  if (toolchain_node) {
    p->toolchain = fatso_yaml_scalar_strdup(toolchain_node);
  }

  int r = 0;

  r = parse_fatso_environment(&p->base_environment, doc, root, out_error_message);
  if (r != 0) goto out;

  yaml_node_t* environments_node = fatso_yaml_mapping_lookup(doc, root, "environments");
  if (environments_node && environments_node->type == YAML_SEQUENCE_NODE) {
    size_t len = fatso_yaml_sequence_length(environments_node);
    p->environments.size = len;
    p->environments.data = fatso_calloc(len, sizeof(struct fatso_environment));
    for (size_t i = 0; i < len; ++i) {
      r = parse_fatso_environment(&p->environments.data[i], doc, fatso_yaml_sequence_lookup(doc, environments_node, i), out_error_message);
      if (r != 0)
        goto out;
    }
  }

out:
  return r;
}

static int
parse_fatso_yml(struct fatso_project* p, const char* file, char** out_error_message) {
  int r = 1;
  yaml_parser_t parser;
  yaml_document_t doc;

  yaml_parser_initialize(&parser);

  FILE* fp = fopen(file, "r");
  if (!fp) {
    *out_error_message = strdup(strerror(errno));
    goto out;
  }

  yaml_parser_set_input_file(&parser, fp);
  if (!yaml_parser_load(&parser, &doc)) {
    *out_error_message = strdup(parser.problem);
    goto out_doc;
  }

  r = parse_fatso_project(p, &doc, out_error_message);
out_doc:
  yaml_document_delete(&doc);
out:
  yaml_parser_delete(&parser);
  fclose(fp);
  return r;
}

int
fatso_load_project(struct fatso* f) {
  int r = 0;
  char* fatso_yml = NULL;
  char* error_message = NULL;

  if (f->project != NULL) {
    // Project is already loaded.
    goto out;
  }

  f->project = fatso_alloc(sizeof(struct fatso_project));
  fatso_project_init(f->project);
  f->project->path = find_fatso_yml(f->working_dir);

  if (!f->project->path) {
    fprintf(stderr, "Error: fatso.yml not found in this or parent directories. Aborting.\n");
    goto error;
  }

  asprintf(&fatso_yml, "%s/fatso.yml", f->project->path);
  r = parse_fatso_yml(f->project, fatso_yml, &error_message);
  if (r != 0) {
    fprintf(stderr, "Cannot load project: %s\n", error_message);
    goto error;
  }

out:
  fatso_free(error_message);
  fatso_free(fatso_yml);
  return r;
error:
  r = 1;
  fatso_unload_project(f);
  goto out;
}

int
fatso_load_or_generate_dependency_graph(struct fatso* f) {
  int r;

  r = fatso_load_dependency_graph(f);
  if (r != 0) {
    r = fatso_generate_dependency_graph(f);
  }
  return r;
}

int
fatso_load_dependency_graph(struct fatso* f) {
  fprintf(stderr, "%s: NIY\n", __func__);
  return -1;
}

int fatso_generate_dependency_graph(struct fatso* f) {
  fprintf(stderr, "%s: NIY\n", __func__);

  // struct fatso_dependency_graph* candidate = fatso_dependency_graph_new();
  // for (size_t i = 0; i < f->project->dependencies.size; ++i) {

  // }
  // int r = fatso_dependency_graph_resolve(candidate);
  // fatso_dependency_graph_destroy(candidate);

  // Add dependencies with constraints to open set.
  // For each dependency:
  //    For each version, starting with newest viable:
  //        Create new open set as a duplicate of the open set
  //        For each dependency:
  //            If the dependency is already in the open set:
  //                If the constraint overlaps:
  //                    Merge the constraints
  //                Else:
  //                    Not viable.
  //            Else:
  //                Add to the open set, recurse.
  return -1;
}
