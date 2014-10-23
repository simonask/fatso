#include <stdio.h>
#include <dlfcn.h>
#include <string.h>

#define FOREACH_OVERRIDE(X) \
  X( 0, fatso_alloc) \
  X( 1, fatso_calloc) \
  X( 2, fatso_build) \
  X( 3, fatso_download) \
  X( 4, fatso_env) \
  X( 5, fatso_exec) \
  X( 6, fatso_file_exists) \
  X( 7, fatso_free) \
  X( 8, fatso_get_homedir) \
  X( 9, fatso_get_number_of_cpu_cores) \
  X(10, fatso_guess_toolchain) \
  X(11, fatso_help) \
  X(12, fatso_home_directory) \
  X(13, fatso_info) \
  X(14, fatso_install) \
  X(15, fatso_install_dependencies) \
  X(16, fatso_logf) \
  X(17, fatso_logz) \
  X(18, fatso_mkdir_p) \
  X(19, fatso_package_build) \
  X(20, fatso_package_build_path) \
  X(21, fatso_package_build_with_output) \
  X(22, fatso_package_download) \
  X(23, fatso_process_start) \
  X(24, fatso_process_wait_all) \
  X(25, fatso_process_wait) \
  X(26, fatso_reallocf) \
  X(27, fatso_sync) \
  X(28, fatso_system) \
  X(29, fatso_system_defer_output_until_error) \
  X(30, fatso_system_with_callbacks) \
  X(31, fatso_system_with_capture) \
  X(32, fatso_unload_project) \
  X(33, fatso_upgrade)
#define NUM_OVERRIDES 34

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
