#include <stdio.h>
#include <dlfcn.h>
#include <string.h>

#define FOREACH_OVERRIDE(X) \
  X(0, fatso_alloc)
#define NUM_OVERRIDES 1

struct function_override {
  const char* symbol;
  void* original;
  void* overridden;
};

static __attribute__((visibility("hidden")))
struct function_override g_overrides[NUM_OVERRIDES] = {{0}};

#define OVERRIDE_TRAMPOLINE(IDX, SYMBOL) \
  void __attribute__((naked)) SYMBOL(void) { \
    __asm__("movq %0, %%rax \n jmpq *%%rax\n" : /*output*/ : /*input*/ "m"(g_overrides[IDX].overridden) : /*clobbered*/ "%rax"); \
  }
FOREACH_OVERRIDE(OVERRIDE_TRAMPOLINE)
#undef OVERRIDE_TRAMPOLINE

void* fatso_intercept_(const char* symbol, void* new_function) {
  for (size_t i = 0; i < NUM_OVERRIDES; ++i) {
    if (strcmp(g_overrides[i].symbol, symbol) == 0) {
      struct function_override* o = &g_overrides[i];
      o->overridden = new_function;
      return o->original;
    }
  }
  return NULL;
}

void fatso_interceptor_reset_(void) {
  #define RESET_TRAMPOLINE(IDX, SYMBOL) \
    g_overrides[IDX].overridden = g_overrides[IDX].original;
  FOREACH_OVERRIDE(RESET_TRAMPOLINE)
  #undef RESET_TRAMPOLINE
}

void __attribute__((constructor))
init(void) {
  #define SETUP_INTERCEPTOR(IDX, SYMBOL) \
    g_overrides[IDX].symbol = #SYMBOL; \
    g_overrides[IDX].original = dlsym(RTLD_NEXT, #SYMBOL); \
    g_overrides[IDX].overridden = g_overrides[IDX].original;
  FOREACH_OVERRIDE(SETUP_INTERCEPTOR)
  #undef SETUP_INTERCEPTOR
}
