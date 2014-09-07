#include "util.h"
#include "internal.h"

#include <sys/stat.h> // lstat
#include <unistd.h>   // getuid
#include <stdlib.h>   // getenv
#include <errno.h>    // errno
#include <stdio.h>    // perror
#include <pwd.h>      // getpwuid
#include <string.h>   // memcpy
#include <inttypes.h> // uint8_t
#include <stdarg.h>

#define BLACK   "\033[22;30m"
#define RED     "\033[01;31m"
#define GREEN   "\033[01;32m"
#define MAGENTA "\033[01;35m"
#define CYAN    "\033[01;36m"
#define YELLOW  "\033[01;33m"
#define RESET   "\033[00m"

typedef uint8_t byte;

void
debugf_(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char* buffer;
  vasprintf(&buffer, fmt, ap);
  va_end(ap);
  fprintf(stderr, YELLOW "DEBUG:" RESET " %s\n", buffer);
  free(buffer);
}

const char*
fatso_get_homedir() {
  const char* homedir = getenv("HOME");
  if (homedir == NULL) {
    struct passwd* pw = getpwuid(getuid());
    homedir = pw->pw_dir;
  }
  return homedir;
}

bool
fatso_directory_exists(const char* path) {
  struct stat s;
  int r = lstat(path, &s);

  if (r != 0) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return false;
    } else {
      perror("lstat");
      exit(1);
    }
  }

  return S_ISDIR(s.st_mode);
}

bool
fatso_file_exists(const char* path) {
  struct stat s;
  int r = lstat(path, &s);

  if (r != 0) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return false;
    } else {
      perror("lstat");
      exit(1);
    }
  }

  return !(S_ISDIR(s.st_mode));
}

void*
fatso_push_back_(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t element_size) {
  size_t old_size = *inout_num_elements;
  size_t new_size = old_size + 1;
  byte* new_data = fatso_reallocf(*inout_data, new_size * element_size);
  *inout_data = new_data;
  *inout_num_elements = new_size;
  byte* ptr = new_data + (old_size * element_size);
  memcpy(ptr, new_element, element_size);
  return ptr;
}

void*
fatso_lower_bound(const void* key, void* base, size_t nel, size_t width, int(*compare)(const void*, const void*)) {
  int r;
  return fatso_lower_bound_cmp(key, base, nel, width, compare, &r);
}

void*
fatso_lower_bound_cmp(const void* key, void* base, size_t nel, size_t width, int(*compare)(const void*, const void*), int* out_cmp) {
  int r = 1; // This becomes the result when there are no elements in the input.

  byte* p = base;
  ssize_t i0 = 0;
  ssize_t i1 = nel;
  ssize_t i = nel / 2;
  while (i0 < i1) {
    byte* k = p + (i * width);
    r = compare(key, k);
    if (r > 0) {
      i0 = i + 1;
    } else if (r < 0) {
      i1 = i;
    } else {
      // element is exactly equal
      break;
    }
    ssize_t nrem = (ssize_t)i1 - (ssize_t)i0;
    i = i0 + (nrem / 2);
  }
  *out_cmp = r;
  return p + (i * width);
}

void*
fatso_bsearch(const void* key, void* base, size_t nel, size_t width, int(*compare)(const void*, const void*)) {
  int cmp;
  byte* end = ((byte*)base) + nel * width;
  byte* p = fatso_lower_bound_cmp(key, base, nel, width, compare, &cmp);
  if (p < end && cmp == 0) {
    return p;
  } else {
    return NULL;
  }
}

void*
fatso_multiset_insert(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t width, int(*compare)(const void*, const void*)) {
  size_t old_size = *inout_num_elements;
  size_t new_size = old_size + 1;
  byte* new_data = fatso_reallocf(*inout_data, new_size * width);
  *inout_data = new_data;
  *inout_num_elements = new_size;
  byte* ptr = fatso_lower_bound(new_element, new_data, old_size, width, compare);

  // Shift old data right:
  byte* old_data_end = new_data + (old_size * width);
  size_t to_move = old_data_end - ptr;
  memmove(ptr + width, ptr, to_move);

  // Insert new element
  memcpy(ptr, new_element, width);
  return ptr;
}

void*
fatso_set_insert(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t width, int(*compare)(const void*, const void*)) {
  byte* old_data = *inout_data;
  size_t old_size = *inout_num_elements;
  int cmp;
  byte* ptr = fatso_lower_bound_cmp(new_element, old_data, old_size, width, compare, &cmp);
  if (cmp != 0) {
    // Expand array:
    size_t new_size = old_size + 1;
    byte* new_data = fatso_reallocf(old_data, new_size * width);
    *inout_data = new_data;
    *inout_num_elements = new_size;

    // Translate ptr to new base:
    size_t found_offset = ptr - old_data;
    ptr = new_data + found_offset;

    // Shift old data right:
    byte* new_data_end = new_data + (old_size * width);
    size_t to_move = new_data_end - ptr;
    memmove(ptr + width, ptr, to_move);

    // Insert new element:
    memcpy(ptr, new_element, width);
  }
  return ptr;
}
