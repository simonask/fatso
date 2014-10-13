#include "fatso.h"
#include "util.h"
#include "internal.h"

#include <yaml.h>
#include <errno.h>
#include <ctype.h> // isspace

static char*
project_build_path(struct fatso* f, struct fatso_package* package) {
  return strdup(fatso_project_directory(f));
}

static char*
project_install_prefix(struct fatso* f, struct fatso_package* package) {
  return strdup(fatso_project_directory(f));
}

static const struct fatso_package_vtbl g_project_package_vtbl = {
  .build_path = project_build_path,
  .install_prefix = project_install_prefix,
};

void fatso_project_init(struct fatso_project* p) {
  memset(p, 0, sizeof(*p));
  fatso_package_init(&p->package);
  p->package.vtbl = &g_project_package_vtbl;
}

void
fatso_project_destroy(struct fatso_project* p) {
  fatso_free(p->path);
  p->path = NULL;
  fatso_package_destroy(&p->package);
}

void
fatso_unload_project(struct fatso* f) {
  fatso_project_destroy(f->project);
  fatso_free(f->project);
  f->project = NULL;
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

  yaml_node_t* root = yaml_document_get_root_node(&doc);
  if (root == NULL) {
    *out_error_message = strdup("YAML file does not contain a document");
    goto out_doc;
  }

  r = fatso_package_parse(&p->package, &doc, root, out_error_message);
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
  f->project->path = strdup(fatso_project_directory(f));

  asprintf(&fatso_yml, "%s/fatso.yml", f->project->path);
  r = parse_fatso_yml(f->project, fatso_yml, &error_message);
  if (r != 0) {
    fatso_logf(f, FATSO_LOG_FATAL, "Cannot load project: %s", error_message);
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
  fatso_logf(f, FATSO_LOG_FATAL, "%s: NIY", __func__);
  return -1;
}

int fatso_generate_dependency_graph(struct fatso* f) {
  int r = 0;

  enum fatso_dependency_graph_resolution_status status;
  struct fatso_dependency_graph* graph = fatso_dependency_graph_for_package(f, &f->project->package, &status);

  fatso_strbuf_t msg;
  fatso_strbuf_init(&msg);

  switch (status) {
    case FATSO_DEPENDENCY_GRAPH_CONFLICT: {
      fatso_strbuf_printf(&msg, "The following dependencies could not be simultaneously met:\n");

      fatso_conflicts_t conflicts = {0};
      fatso_dependency_graph_get_conflicts(graph, &conflicts);
      for (size_t i = 0; i < conflicts.size; ++i) {
        struct fatso_dependency* dep = conflicts.data[i].dependency;
        fatso_strbuf_printf(&msg, "  %s ", dep->name);
        for (size_t j = 0; j < dep->constraints.size; ++j) {
          fatso_strbuf_printf(&msg, "%s", fatso_constraint_to_string_unsafe(&dep->constraints.data[j]));
          if (j + 1 < dep->constraints.size) {
            fatso_strbuf_printf(&msg, ", ");
          }
        }
        fatso_strbuf_printf(&msg, " (from %s %s)\n", conflicts.data[i].package->name, fatso_version_string(&conflicts.data[i].package->version));
      }

      r = 1;
      break;
    }
    case FATSO_DEPENDENCY_GRAPH_UNKNOWN: {
      fatso_strbuf_printf(&msg, "The following packages could not be found in any repository:\n");
      fatso_unknown_dependencies_t unknowns = {0};
      fatso_dependency_graph_get_unknown_dependencies(graph, &unknowns);
      for (size_t i = 0; i < unknowns.size; ++i) {
        fatso_strbuf_printf(&msg, "  %s\n", unknowns.data[i]->name);
      }

      r = 1;
      break;
    }
    case FATSO_DEPENDENCY_GRAPH_SUCCESS: {
      fatso_dependency_graph_topological_sort(graph, f, &f->project->install_order.data, &f->project->install_order.size);
      break;
    }
  }
  if (msg.size) {
    char* message = fatso_strbuf_strdup(&msg);
    fatso_logz(f, FATSO_LOG_FATAL, message, msg.size);
    fatso_free(message);
  }
  fatso_strbuf_destroy(&msg);
  fatso_dependency_graph_free(graph);

  return r;
}
