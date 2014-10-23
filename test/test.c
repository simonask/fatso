#include <dlfcn.h>

#include "test.h"
#include "../internal.h"
#include "../util.h"
#include "../fatso.h"

void* fatso_intercept(const char* symbol, void* new_function) {
  static void* real_fatso_intercept = NULL;
  if (!real_fatso_intercept) {
    real_fatso_intercept = dlsym(RTLD_DEFAULT, "fatso_intercept_");
  }
  return ((void*(*)(const char*, void*))real_fatso_intercept)(symbol, new_function);
}

void fatso_interceptor_reset(void) {
  static void* real_fatso_interceptor_reset = NULL;
  if (!real_fatso_interceptor_reset) {
    real_fatso_interceptor_reset = dlsym(RTLD_DEFAULT, "fatso_interceptor_reset_");
  }
  ((void(*)(void))real_fatso_interceptor_reset)();
}

static void*(*orig_fatso_alloc)(size_t) = NULL;
static bool stub_fatso_alloc_called = false;
static void* stub_fatso_alloc(size_t len) {
  stub_fatso_alloc_called = true;
  return orig_fatso_alloc(len);
}

static void test_interceptor() {
  orig_fatso_alloc = (void*(*)(size_t))fatso_intercept("fatso_alloc", (void*)stub_fatso_alloc);
  void* ptr = fatso_alloc(128);
  ASSERT(stub_fatso_alloc_called == true);
  fatso_interceptor_reset();
  stub_fatso_alloc_called = false;
  ptr = fatso_alloc(128);
  ASSERT(stub_fatso_alloc_called == false);
}

static void test_fatso_version_from_string() {
  void* ptr = fatso_alloc(120);
  struct fatso_version ver;
  fatso_version_from_string(&ver, "0.1.2a");
  ASSERT(ver.components.size == 4);
  ASSERT(strcmp(ver.components.data[0], "0") == 0);
  ASSERT(strcmp(ver.components.data[1], "1") == 0);
  ASSERT(strcmp(ver.components.data[2], "2") == 0);
  ASSERT(strcmp(ver.components.data[3], "a") == 0);
  fatso_version_destroy(&ver);

  fatso_version_from_string(&ver, "a.2b");
  ASSERT_FMT(ver.components.size == 3, "got %zu", ver.components.size);
  fatso_version_destroy(&ver);

  fatso_version_from_string(&ver, "a2b");
  ASSERT_FMT(ver.components.size == 3, "got %zu", ver.components.size);
  fatso_version_destroy(&ver);
}

static bool same_side(int a, int b) {
  if (a == 0 && b == 0) return true;
  if (a < 0 && b < 0) return true;
  if (a > 0 && b > 0) return true;
  return false;
}

static void test_fatso_version_compare() {
  struct fatso_version a;
  struct fatso_version b;

  struct version_compare_test_case {
    const char* a;
    const char* b;
    int r;
  };

  static const struct version_compare_test_case test_cases[] = {
    {"123", "123", 0},
    {"abc", "abc", 0},
    {"a.2b", "a2b", 0},
    {"0.1",  "0.2", -1},
    {"0.10", "0.1", 1},
    {"1.a.0", "1.aa.0", -1},
    {"1.2", "2", -1},
    {NULL, NULL, 0}
  };

  for (const struct version_compare_test_case* p = test_cases; p->a; ++p) {
    fatso_version_from_string(&a, p->a);
    fatso_version_from_string(&b, p->b);
    int r = fatso_version_compare(&a, &b);
    ASSERT_FMT(same_side(r, p->r), "cmp(\"%s\", \"%s\") == %d, expected %d", p->a, p->b, r, p->r);
    fatso_version_destroy(&a);
    fatso_version_destroy(&b);
  }
}

static void
test_fatso_append() {
  FATSO_ARRAY(int) a = {0};
  for (int i = 0; i < 5; ++i) {
    fatso_push_back_v(&a, &i);
  }
  const int v[] = {5, 6, 7, 8, 9};
  fatso_append_v(&a, v, 5);
  for (int i = 0; i < 10; ++i) {
    ASSERT_FMT(a.data[i] == i, "Element %d: expected %d to equal %d", i, a.data[i], i);
  }
}

static int compare_ints(const void* a, const void* b) {
  return (*(const int*)a) - (*(const int*)b);
}

static void test_fatso_lower_bound() {
  {
    int v[] = {-1, 2, 3, 4, 4, 4, 5};
    size_t nel = sizeof(v) / sizeof(v[0]);

    int two  = 2;
    int four = 4;
    int six  = 6;

    void* find_2 = fatso_lower_bound(&two, v, nel, sizeof(int), compare_ints);
    ASSERT(find_2 == &v[1]);

    void* find_4 = fatso_lower_bound(&four, v, nel, sizeof(int), compare_ints);
    ASSERT(find_4 == &v[3]);

    void* find_6 = fatso_lower_bound(&six, v, nel, sizeof(int), compare_ints);
    ASSERT(find_6 == &v[nel]);
  }

  {
    int v[] = {1, 2, 8, 9};
    size_t nel = sizeof(v) / sizeof(v[0]);

    int two = 2;
    int cmp;
    void* find_2 = fatso_lower_bound_cmp(&two, v, nel, sizeof(int), compare_ints, &cmp);
    ASSERT(find_2 == &v[1]);
    ASSERT_FMT(cmp == 0, "Expected %d, got %d.", 0, cmp);
  }
}

static void
test_fatso_set_insert() {
  {
    FATSO_ARRAY(int) list = {0};

    int insert[] = {9, 1, 8, 2, 7, 3, 6, 4, 5};
    int expect[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    size_t nel = sizeof(insert) / sizeof(insert[0]);

    for (size_t i = 0; i < nel; ++i) {
      fatso_set_insert_v(&list, &insert[i], compare_ints);
      ASSERT_FMT(list.size == i + 1, "Expected %zu, got %zu.", nel + 1, list.size);
    }

    for (size_t i = 0; i < nel; ++i) {
      ASSERT_FMT(list.data[i] == expect[i], "Expected %d, got %d (at %zu).", expect[i], list.data[i], i);
    }
  }

  {
    FATSO_ARRAY(int) list = {0};

    int insert[] = {1, 9, 2, 8, 2, 8, 3, 4};
    int expect[] = {1, 2, 3, 4, 8, 9};
    size_t ninput = sizeof(insert) / sizeof(insert[0]);
    size_t nexpect = sizeof(expect) / sizeof(expect[0]);

    for (size_t i = 0; i < ninput; ++i) {
      fatso_set_insert_v(&list, &insert[i], compare_ints);
    }

    ASSERT_FMT(list.size == nexpect, "Expected %zu, got %zu.", nexpect, list.size);

    for (size_t i = 0; i < nexpect; ++i) {
      ASSERT_FMT(list.data[i] == expect[i], "Expected %d, got %d (at %zu).", expect[i], list.data[i], i);
    }
  }
}

static void
test_fatso_multiset_insert() {
  FATSO_ARRAY(int) list = {0};

  int insert[] = {9, 1, 8, 2, 7, 3, 6, 4, 5};
  int expect[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  size_t nel = sizeof(insert) / sizeof(insert[0]);

  for (size_t i = 0; i < nel; ++i) {
    fatso_multiset_insert_v(&list, &insert[i], compare_ints);
    ASSERT_FMT(list.size == i + 1, "Expected %zu, got %zu.", nel + 1, list.size);
  }

  for (size_t i = 0; i < nel; ++i) {
    ASSERT_FMT(list.data[i] == expect[i], "Expected %d, got %d (at %zu).", expect[i], list.data[i], i);
  }
}

static void
test_fatso_version_matches_constraint() {
  struct version_match_test_case {
    const char* constraint;
    bool match_expectation;
  };

  {
    struct fatso_version ver;
    fatso_version_from_string(&ver, "1.2.3");

    static const struct version_match_test_case cases[] = {
      {"= 1.2.3", true},
      {"> 1.2.2", true},
      {"> 1.2", true},
      {"> 1", true},
      {"> 2", false},
      {">= 1.2.3", true},
      {">= 1.2.2", true},
      {">= 2", false},
      {"< 2", true},
      {"< 1", false},
      {"~> 1.2.4", true},
      {"~> 1.2", true},
      {"~> 2.3", false},
      {"~> 2", true},
      {NULL, 0}
    };

    for (const struct version_match_test_case* p = cases; p->constraint; ++p) {
      struct fatso_constraint c;
      fatso_constraint_from_string(&c, p->constraint);
      ASSERT_FMT(fatso_version_matches_constraints(&ver, &c, 1) == p->match_expectation, "Expected '%s' %s match constraint '%s'.", fatso_version_string(&ver), p->match_expectation ? "to" : "to not", p->constraint);
    }
  }
}

static void
test_fatso_exec() {
  setenv("FOO", "test", 1);
  char* output;
  size_t output_len;
  int r = fatso_system_with_capture("echo $FOO", &output, &output_len);
  ASSERT(r == 0);
  ASSERT(strncmp("test\n", output, output_len) == 0);
}

int main(int argc, char const *argv[])
{
  void* interceptor_found = dlsym(RTLD_DEFAULT, "fatso_intercept_");
  if (!interceptor_found) {
    fprintf(stderr, RED "Function interceptor not found! Please run with libtest-interceptor preloaded." RESET "\nOn Darwin:\n\tenv DYLD_INSERT_LIBRARIES=test/libtest-interceptor.dylib %s\nOn Linux:\n\tenv LD_PRELOAD=test/libtest-interceptor.so %s\n", argv[0], argv[0]);
    _exit(1);
  }

  TEST(test_interceptor);
  TEST(test_fatso_version_from_string);
  TEST(test_fatso_version_compare);
  TEST(test_fatso_append);
  TEST(test_fatso_lower_bound);
  TEST(test_fatso_set_insert);
  TEST(test_fatso_multiset_insert);
  TEST(test_fatso_version_matches_constraint);
  TEST(test_fatso_exec);
  return g_any_test_failed;
}
