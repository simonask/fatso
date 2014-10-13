#include "fatso.h"
#include "internal.h"

#include <string.h> // memset
#include <yaml.h>
#include <errno.h>
#include <stdio.h> // asprintf

static char*
package_build_path(struct fatso* f, struct fatso_package* p) {
  char* path;
  asprintf(&path, "%s/.fatso/build/%s/%s", fatso_project_directory(f), p->name, fatso_version_string(&p->version));
  return path;
}

static char*
package_install_prefix(struct fatso* f, struct fatso_package* p) {
  char* path;
  asprintf(&path, "%s/.fatso", fatso_project_directory(f));
  return path;
}

char*
fatso_package_build_path(struct fatso* f, struct fatso_package* p) {
  return p->vtbl->build_path(f, p);
}

char*
fatso_package_install_prefix(struct fatso* f, struct fatso_package* p) {
  return p->vtbl->install_prefix(f, p);
}

static const struct fatso_package_vtbl g_default_package_vtbl = {
  .build_path = package_build_path,
  .install_prefix = package_install_prefix,
};

void fatso_package_init(struct fatso_package* p) {
  memset(p, 0, sizeof(*p));
  p->vtbl = &g_default_package_vtbl;
}

void
fatso_package_destroy(struct fatso_package* p) {
  fatso_free(p->name);
  fatso_version_destroy(&p->version);
  fatso_free(p->author);
  fatso_free(p->toolchain);
  fatso_configuration_destroy(&p->base_configuration);
  for (size_t i = 0; i < p->configurations.size; ++i) {
    fatso_configuration_destroy(&p->configurations.data[i]);
  }
  fatso_free(p->configurations.data);
  memset(p, 0, sizeof(*p));
}

int fatso_package_parse(struct fatso_package* p, struct yaml_document_s* doc, struct yaml_node_s* node, char** out_error_message) {
  yaml_node_t* name_node = fatso_yaml_mapping_lookup(doc, node, "project");
  if (name_node) {
    p->name = fatso_yaml_scalar_strdup(name_node);
  }

  yaml_node_t* version_node = fatso_yaml_mapping_lookup(doc, node, "version");
  if (version_node) {
    char* version = fatso_yaml_scalar_strdup(version_node);
    fatso_version_from_string(&p->version, version);
    fatso_free(version);
  }

  yaml_node_t* author_node = fatso_yaml_mapping_lookup(doc, node, "author");
  if (author_node) {
    p->author = fatso_yaml_scalar_strdup(author_node);
  }

  yaml_node_t* toolchain_node = fatso_yaml_mapping_lookup(doc, node, "toolchain");
  if (toolchain_node) {
    p->toolchain = fatso_yaml_scalar_strdup(toolchain_node);
  }

  int r = 0;

  yaml_node_t* source_node = fatso_yaml_mapping_lookup(doc, node, "source");
  if (source_node) {
    p->source = fatso_alloc(sizeof(struct fatso_source));
    r = fatso_source_parse(p->source, doc, source_node, out_error_message);
    if (r != 0) {
      fatso_free(p->source);
      goto out;
    }
  }

  r = fatso_configuration_parse(&p->base_configuration, doc, node, out_error_message);
  if (r != 0)
    goto out;

  yaml_node_t* configurations_node = fatso_yaml_mapping_lookup(doc, node, "configurations");
  if (configurations_node && configurations_node->type == YAML_SEQUENCE_NODE) {
    size_t len = fatso_yaml_sequence_length(configurations_node);
    p->configurations.size = len;
    p->configurations.data = fatso_calloc(len, sizeof(struct fatso_configuration));
    for (size_t i = 0; i < len; ++i) {
      r = fatso_configuration_parse(&p->configurations.data[i], doc, fatso_yaml_sequence_lookup(doc, configurations_node, i), out_error_message);
      if (r != 0)
        goto out;
    }
  }

out:
  return r;
}

int
parse_package_from_document(struct fatso_package* p, struct yaml_document_s* doc, char** out_error_message) {
  yaml_node_t* root = yaml_document_get_root_node(doc);
  if (root == NULL) {
    *out_error_message = strdup("YAML file does not contain a document.");
    return 1;
  }
  return fatso_package_parse(p, doc, root, out_error_message);
}

int
fatso_package_parse_from_file(struct fatso_package* p, FILE* fp, char** out_error_message) {
  int r = 1;
  yaml_parser_t parser;
  yaml_document_t doc;
  yaml_parser_initialize(&parser);
  yaml_parser_set_input_file(&parser, fp);
  if (!yaml_parser_load(&parser, &doc)) {
    *out_error_message = strdup(strerror(errno));
    goto out;
  }

  r = parse_package_from_document(p, &doc, out_error_message);
  yaml_document_delete(&doc);
out:
  yaml_parser_delete(&parser);
  return r;
}

int
fatso_package_parse_from_string(struct fatso_package* p, const char* buffer, char** out_error_message) {
  int r = 1;
  yaml_parser_t parser;
  yaml_document_t doc;
  yaml_parser_initialize(&parser);
  yaml_parser_set_input_string(&parser, (const unsigned char*)buffer, strlen(buffer));
  if (!yaml_parser_load(&parser, &doc)) {
    *out_error_message = strdup(strerror(errno));
    goto out;
  }

  r = parse_package_from_document(p, &doc, out_error_message);
  yaml_document_delete(&doc);
out:
  yaml_parser_delete(&parser);
  return r;
}
