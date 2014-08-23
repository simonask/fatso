#include "internal.h"

#include <string.h>
#include <stdlib.h> // free

void
fatso_define_init(struct fatso_define* def, const char* name, const char* value) {
  def->key = strdup(name);
  def->value = strdup(value);
}

void
fatso_define_destroy(struct fatso_define* def) {
  fatso_free(def->key);
  fatso_free(def->value);
}
