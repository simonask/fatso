#include "test.h"
#include "../internal.h"

static void test_fatso_version_from_string() {
  struct fatso_version ver;
  fatso_version_from_string(&ver, "0.1.2a");
  ASSERT(ver.num_components == 4);
  ASSERT(strcmp(ver.components[0], "0") == 0);
  ASSERT(strcmp(ver.components[1], "1") == 0);
  ASSERT(strcmp(ver.components[2], "2") == 0);
  ASSERT(strcmp(ver.components[3], "a") == 0);
  fatso_version_destroy(&ver);

  fatso_version_from_string(&ver, "a.2b");
  ASSERT_FMT(ver.num_components == 3, "got %zu", ver.num_components);
  fatso_version_destroy(&ver);

  fatso_version_from_string(&ver, "a2b");
  ASSERT_FMT(ver.num_components == 3, "got %zu", ver.num_components);
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

int main(int argc, char const *argv[])
{
  TEST(test_fatso_version_from_string);
  TEST(test_fatso_version_compare);
  return g_any_test_failed;
}
