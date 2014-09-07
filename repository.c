#include "fatso.h"
#include "internal.h"
#include "util.h"

#include <glob.h>
#include <stdio.h>  // perror
#include <string.h> // strcmp
#include <stdlib.h> // qsort

struct fatso_package_versions_list {
  char* name;
  FATSO_ARRAY(struct fatso_package) versions;
};

typedef FATSO_ARRAY(struct fatso_package_versions_list) fatso_package_versions_list_t;

int compare_string_to_versions_list(const void* a, const void* b) {
  const char* str = a;
  const struct fatso_package_versions_list* p = b;
  return strcmp(str, p->name);
}

int compare_versions_lists(const void* a, const void* b) {
  const struct fatso_package_versions_list* pa = a;
  const struct fatso_package_versions_list* pb = b;
  return strcmp(pa->name, pb->name);
}

int compare_packages_by_version(const void* a, const void* b) {
  const struct fatso_package* pa = a;
  const struct fatso_package* pb = b;
  return fatso_version_compare(&pa->version, &pb->version);
}

static void
package_guess_version_from_filename(struct fatso_package* package, const char* path) {
  const char* p0 = strrchr(path, '/') + 1;
  const char* p1 = strrchr(path, '.');
  if (p1 > p0) {
    size_t len = p1 - p0;
    char* buffer = alloca(len + 1);
    memcpy(buffer, p0, len);
    buffer[len] = '\0';

    fatso_version_destroy(&package->version);
    fatso_version_from_string(&package->version, buffer);
  }
}

ssize_t
fatso_repository_find_package_versions(struct fatso* f, const char* name, struct fatso_package** out_packages) {
  static fatso_package_versions_list_t* g_versions_cache = NULL;

  if (g_versions_cache == NULL) {
    g_versions_cache = fatso_alloc(sizeof(fatso_package_versions_list_t));
  }

  struct fatso_package_versions_list* list;
  list = fatso_bsearch_v(name, g_versions_cache, compare_string_to_versions_list);

  if (list == NULL) {
    struct fatso_package_versions_list l = {
      .name = strdup(name),
      .versions = {0},
    };
    list = fatso_set_insert_v(g_versions_cache, &l, compare_versions_lists);

    // Check 'packages' dir:
    int r = 0;
    char* package_dir = NULL;
    char* pattern = NULL;
    asprintf(&package_dir, "%s/packages/%s", fatso_home_directory(f), name);
    if (!fatso_directory_exists(package_dir)) {
      goto error;
    }

    // Find all packages in the dir:
    glob_t g;
    asprintf(&pattern, "%s/*.yml", package_dir);
    r = glob(pattern, GLOB_NOSORT, NULL, &g);
    if (r != 0) {
      perror("glob");
      goto error;
    }

    // Allocate the memory for packages:
    struct fatso_package* packages = fatso_calloc(g.gl_pathc, sizeof(struct fatso_package));
    size_t valid_i = 0;

    // Load all package versions:
    for (size_t i = 0; i < g.gl_pathc; ++i) {
      const char* path = g.gl_pathv[i];
      struct fatso_package* package = &packages[valid_i];

      if (fatso_file_exists(path)) {
        FILE* fp = fopen(path, "r");
        if (fp) {
          fatso_package_init(package);
          char* error_message;
          r = fatso_package_parse_from_file(package, fp, &error_message);
          if (r == 0) {
            if (fatso_version_string(&package->version) == NULL) {
              package_guess_version_from_filename(package, path);
            }
            ++valid_i;
          } else {
            fprintf(stderr, "WARNING (%s): %s\n", path, error_message);
            free(error_message);
            fatso_package_destroy(package);
          }
          fclose(fp);
        } else {
          fprintf(stderr, "WARNING (%s): Could not open file.\n", path);
        }
      }
    }
    globfree(&g);

    // Sort packages by version:
    qsort(packages, valid_i, sizeof(struct fatso_package), compare_packages_by_version);
    list->versions.data = packages;
    list->versions.size = valid_i;

    goto out;
error:
    r = -1;
out:
    free(package_dir);
    free(pattern);
    if (r < 0) return r;
  }

  *out_packages = list->versions.data;
  return list->versions.size;
}

enum fatso_repository_result
fatso_repository_find_package_matching_dependency(struct fatso* f, struct fatso_dependency* dep, struct fatso_version* less_than_version, struct fatso_package** out_package) {
  struct fatso_package* packages = NULL;
  ssize_t num_versions = fatso_repository_find_package_versions(f, dep->name, &packages);
  if (num_versions >= 0) {
    if (num_versions > 0) {
      struct fatso_package* p = &packages[num_versions - 1];

      if (less_than_version) {
        while (p >= packages && fatso_version_compare(&p->version, less_than_version) >= 0) {
          --p;
        }
      }

      while (p >= packages) {
        if (fatso_version_matches_constraints(&p->version, dep->constraints.data, dep->constraints.size)) {
          *out_package = p;
          return FATSO_PACKAGE_OK;
        }
        --p;
      }
    }

    return FATSO_PACKAGE_NO_MATCHING_VERSION;
  } else {
    return FATSO_PACKAGE_UNKNOWN;
  }
}

enum fatso_repository_result
fatso_repository_find_package(struct fatso* f, const char* name, struct fatso_version* less_than_version, struct fatso_package** out_package) {
  struct fatso_dependency dep = {
    .name = (char*)name,
    .constraints = {0},
  };

  return fatso_repository_find_package_matching_dependency(f, &dep, less_than_version, out_package);
}
