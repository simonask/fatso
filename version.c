#include "internal.h"
#include "util.h"

#include <string.h>
#include <ctype.h>  // isalpha, isspace
#include <stdlib.h> // atoi

const char*
fatso_version_requirement_to_string(enum fatso_version_requirement req) {
  switch (req) {
    case FATSO_VERSION_ANY: return "*";
    case FATSO_VERSION_LT: return "<";
    case FATSO_VERSION_LE: return "<=";
    case FATSO_VERSION_EQ: return "=";
    case FATSO_VERSION_GT: return ">";
    case FATSO_VERSION_GE: return ">=";
    case FATSO_VERSION_APPROXIMATELY: return "~>";
  }
}

void
fatso_version_init(struct fatso_version* ver) {
  memset(ver, 0, sizeof(*ver));
}

void
fatso_version_copy(struct fatso_version* a, const struct fatso_version* b) {
  fatso_version_destroy(a);
  a->string = b->string ? strdup(b->string) : NULL;
  for (size_t i = 0; i < b->components.size; ++i) {
    fatso_version_append_component(a, b->components.data[i], strlen(b->components.data[i]));
  }
}

void
fatso_version_destroy(struct fatso_version* ver) {
  for (size_t i = 0; i < ver->components.size; ++i) {
    fatso_free(ver->components.data[i]);
  }
  fatso_free(ver->components.data);
  ver->components.data = NULL;
  ver->components.size = 0;
  fatso_free(ver->string);
}

void
fatso_version_append_component(struct fatso_version* ver, const char* component, size_t n) {
  if (n == 0) return;
  char* append = strndup(component, n);
  fatso_push_back_v(&ver->components, &append);
}

int
fatso_version_from_string(struct fatso_version* ver, const char* str) {
  enum {
    INITIAL,
    ALPHA,
    NUMERIC,
  } state = INITIAL;
  ver->components.data = NULL;
  ver->components.size = 0;
  const char* component_start = str;
  const char* p = str;

  for (; *p; ++p) {
    int c = *p;
    if (c >= '0' && c <= '9') {
      switch (state) {
        case INITIAL: state = NUMERIC; break;
        case NUMERIC: break;
        case ALPHA: {
          fatso_version_append_component(ver, component_start, p - component_start);
          component_start = p;
          state = NUMERIC;
          break;
        }
      }
    } else if (isalpha(c)) {
      switch (state) {
        case INITIAL: state = ALPHA; break;
        case ALPHA: break;
        case NUMERIC: {
          fatso_version_append_component(ver, component_start, p - component_start);
          component_start = p;
          state = ALPHA;
          break;
        }
      }
    } else {
      // Non-alphanumeric means breaking a component.
      if (component_start != p) {
        fatso_version_append_component(ver, component_start, p - component_start);
      }
      component_start = p+1;
    }
  }
  // Append trailing:
  fatso_version_append_component(ver, component_start, p - component_start);

  ver->string = strdup(str);
  return 0;
}

int
fatso_version_compare_n_components(const struct fatso_version* a, const struct fatso_version* b, size_t n) {
  if (a->components.size < n)
    n = a->components.size;
  if (b->components.size < n)
    n = b->components.size;

  int r;

  for (size_t i = 0; i < n; ++i) {
    const char* ac = a->components.data[i];
    const char* bc = b->components.data[i];
    if (isalpha(*ac) && isalpha(*bc)) {
      // Both are alphanumeric, use strcmp:
      r = strcmp(ac, bc);
      if (r != 0)
        return r;
    } else {
      // One isn't alphanumeric -- put that one first:
      if (isalpha(*ac)) {
        return 1;
      } else if (isalpha(*bc)) {
        return -1;
      } else {
        // Both are numeric, compare numbers:
        int ai = atoi(ac);
        int bi = atoi(bc);
        r = ai - bi;
        if (r != 0)
          return r;
      }
    }
  }

  // Everything was equal so far:
  return 0;
}

int
fatso_version_compare(const struct fatso_version* a, const struct fatso_version* b) {
  size_t n = a->components.size < b->components.size ? a->components.size : b->components.size;
  int r = fatso_version_compare_n_components(a, b, n);
  if (r == 0) {
    // Everything else was equal, so compare the number of components, where the
    // shortest comes first:
    return ((int)a->components.size - (int)b->components.size);
  }
  return r;
}

int
fatso_version_compare_t(void* thunk, const void* a, const void* b) {
  return fatso_version_compare(a, b);
}

const char*
fatso_version_string(const struct fatso_version* ver) {
  return ver->string;
}

void
fatso_constraint_destroy(struct fatso_constraint* c) {
  fatso_version_destroy(&c->version);
}

int
fatso_constraint_from_string(struct fatso_constraint* c, const char* str) {
  const char* p = str;

  while (*p && isspace(*p)) ++p;

  switch (*p) {
    case '~': {
      if (p[1] == '>') {
        c->version_requirement = FATSO_VERSION_APPROXIMATELY;
        p += 2;
        break;
      }
    }
    case '>': {
      if (p[1] == '=') {
        c->version_requirement = FATSO_VERSION_GE;
        ++p;
      } else {
        c->version_requirement = FATSO_VERSION_GT;
      }
      ++p;
      break;
    }
    case '<': {
      if (p[1] == '=') {
        c->version_requirement = FATSO_VERSION_LE;
        ++p;
      } else {
        c->version_requirement = FATSO_VERSION_LT;
      }
      ++p;
      break;
    }
    case '=': {
      c->version_requirement = FATSO_VERSION_EQ;
      ++p;
      break;
    }
    default: break;
  }
  while (*p && isspace(*p)) ++p;


  return fatso_version_from_string(&c->version, p);
}
