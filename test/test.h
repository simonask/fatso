#pragma once
#ifndef FATSO_TEST_H_INCLUDED
#define FATSO_TEST_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h> // free
#include <stdarg.h>

#define TEST(func) run_test(func, #func)
#define ASSERT(expr) ASSERT_FMT(expr, NULL)
#define ASSERT_FMT(expr, ...) { \
  ++g_num_asserts; \
  if (!(expr)) { \
    g_failed_expression = #expr; \
    g_failed_file = __FILE__; \
    g_failed_line_number = __LINE__; \
    asprintf_if(&g_failed_info, __VA_ARGS__); \
    ++g_test_failed; \
    return; \
  } \
}

static void asprintf_if(char** out_buffer, const char* fmt, ...) {
  if (fmt) {
    va_list ap;
    va_start(ap, fmt);
    vasprintf(out_buffer, fmt, ap);
    va_end(ap);
  }
}

static int g_test_failed = 0;
static int g_num_asserts = 0;
static const char* g_failed_expression = NULL;
static const char* g_failed_file = NULL;
static int g_failed_line_number = 0;
static int g_any_test_failed = 0;
static char* g_failed_info = NULL;

#define BLACK   "\033[22;30m"
#define RED     "\033[01;31m"
#define GREEN   "\033[01;32m"
#define MAGENTA "\033[01;35m"
#define CYAN    "\033[01;36m"
#define YELLOW  "\033[01;33m"
#define RESET   "\033[00m"

static void run_test(void(*f)(), const char* name) {
  static const char padding[] = "........................................................................";
  int target_len = 70;
  int len = strlen(name);
  int pad_len = target_len - len;
  if (pad_len < 0) pad_len = 0;
  printf("%s%*.*s ", name, pad_len, pad_len, padding);
  g_test_failed = 0;
  g_num_asserts = 0;
  f();
  if (g_test_failed) {
    g_any_test_failed = 1;
    printf(RED "fail" RESET "\nLocation: %s:%d\nAssertion: %s\n", g_failed_file, g_failed_line_number, g_failed_expression);
    if (g_failed_info) {
      printf("Info: %s\n", g_failed_info);
      free(g_failed_info);
      g_failed_info = NULL;
    }
  } else {
    if (g_num_asserts == 0) {
      printf(YELLOW "pending\n" RESET);
    } else {
      printf(GREEN "ok\n" RESET);
    }
  }
}


#endif // FATSO_TEST_H_INCLUDED
