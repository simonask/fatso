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
  free(p->path);
  free(p->name);
  fatso_version_destroy(&p->version);
  free(p->author);
  free(p->toolchain);
  fatso_environment_destroy(&p->base_environment);
  for (size_t i = 0; i < p->num_environments; ++i) {
    fatso_environment_destroy(&p->environments[i]);
  }
  free(p->environments);
  memset(p, 0, sizeof(*p));
}

void
fatso_unload_project(struct fatso* f) {
  fatso_project_destroy(f->project);
  free(f->project);
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
  free(check);
  return r;
not_found:
  free(r);
  free(check);
  return NULL;
}

static int
parse_version_requirement(struct fatso_dependency* dep, const char* str, char** out_error_message) {
  const char* p = str;

  while (*p && isspace(*p)) ++p;

  switch (*p) {
    case '~': {
      if (p[1] == '>') {
        dep->version_requirement = FATSO_VERSION_APPROXIMATELY;
        p += 2;
        break;
      }
    }
    case '>': {
      if (p[1] == '=') {
        dep->version_requirement = FATSO_VERSION_GE;
        ++p;
      } else {
        dep->version_requirement = FATSO_VERSION_GT;
      }
      ++p;
      break;
    }
    case '<': {
      if (p[1] == '=') {
        dep->version_requirement = FATSO_VERSION_LE;
        ++p;
      } else {
        dep->version_requirement = FATSO_VERSION_LT;
      }
      ++p;
      break;
    }
    case '=': {
      dep->version_requirement = FATSO_VERSION_EQ;
      ++p;
      break;
    }
    default: break;
  }
  while (*p && isspace(*p)) ++p;


  return fatso_version_from_string(&dep->version, p);
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
  dep->name = fatso_yaml_scalar_strdup(package_name);

  yaml_node_t* version_or_options = fatso_yaml_sequence_lookup(doc, node, 1);
  if (version_or_options == NULL) {
    dep->version_requirement = FATSO_VERSION_ANY;
  } else if (version_or_options->type == YAML_SCALAR_NODE) {
    version_requirement_string = fatso_yaml_scalar_strdup(version_or_options);
    r = parse_version_requirement(dep, version_requirement_string, out_error_message);
    if (r != 0)
      goto error;
  } else {
    *out_error_message = strdup("Invalid version/options!");
    goto out;
  }

out:
  free(version_requirement_string);
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
      e->num_dependencies = len;
      e->dependencies = calloc(len, sizeof(struct fatso_dependency));
      for (size_t i = 0; i < len; ++i) {
        r = parse_fatso_dependency(&e->dependencies[i], doc, fatso_yaml_sequence_lookup(doc, dependencies, i), out_error_message);
        if (r != 0)
          goto out;
      }
    }
  }

  yaml_node_t* defines = fatso_yaml_mapping_lookup(doc, node, "defines");
  if (defines && defines->type == YAML_MAPPING_NODE) {
    size_t len = fatso_yaml_mapping_length(defines);
    if (len != 0) {
      e->num_defines = len;
      e->defines = calloc(len, sizeof(struct fatso_define));
      for (size_t i = 0; i < len; ++i) {
        yaml_node_pair_t* pair = &defines->data.mapping.pairs.start[i];
        yaml_node_t* key = yaml_document_get_node(doc, pair->key);
        yaml_node_t* value = yaml_document_get_node(doc, pair->value);
        e->defines[i].key = fatso_yaml_scalar_strdup(key);
        e->defines[i].value = fatso_yaml_scalar_strdup(value);
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
    free(version);
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
    p->num_environments = len;
    p->environments = calloc(len, sizeof(struct fatso_environment));
    for (size_t i = 0; i < len; ++i) {
      r = parse_fatso_environment(&p->environments[i], doc, fatso_yaml_sequence_lookup(doc, environments_node, i), out_error_message);
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

  f->project = malloc(sizeof(struct fatso_project));
  fatso_project_init(f->project);
  f->project->path = find_fatso_yml(f->working_dir);

  if (!f->project->path) {
    goto error;
  }

  asprintf(&fatso_yml, "%s/fatso.yml", f->project->path);
  r = parse_fatso_yml(f->project, fatso_yml, &error_message);
  if (r != 0) {
    fprintf(stderr, "Cannot load project: %s\n", error_message);
    goto error;
  }

out:
  free(error_message);
  free(fatso_yml);
  return r;
error:
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
  return -1;
}
