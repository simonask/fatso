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

typedef uint8_t byte;

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

  if (errno == ENOENT || errno == ENOTDIR) {
    return false;
  }

  if (r != 0) {
    perror("lstat");
    exit(1);
  }

  return S_ISDIR(s.st_mode);
}

bool
fatso_file_exists(const char* path) {
  struct stat s;
  int r = lstat(path, &s);

  if (errno == ENOENT || errno == ENOTDIR) {
    return false;
  }

  if (r != 0) {
    perror("lstat");
    exit(1);
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
  byte* p0 = base;
  byte* p1 = p0 + (nel * width);
  byte* p  = p0 + ((nel / 2) * width);
  while (p1 > p0) {
    int r = *out_cmp = compare(key, p);
    if (r > 0) {
      p0 = p + width;
    } else {
      // r <= 0
      p1 = p;
    }
    int nrem = (p1 - p0) / width;
    p = p0 + (nrem / 2) * width;
  }
  return p;
}

void*
fatso_insert_ordered(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t width, int(*compare)(const void*, const void*)) {
  size_t old_size = *inout_num_elements;
  size_t new_size = old_size + 1;
  byte* new_data = fatso_reallocf(*inout_data, new_size * width);
  *inout_data = new_data;
  *inout_num_elements = new_size;
  byte* ptr = fatso_lower_bound(new_element, new_data, old_size, width, compare);

  // Shift old data right:
  byte* old_data_end = new_data + (old_size * width);
  size_t to_move = (old_data_end - ptr);
  memmove(ptr + width, ptr, to_move);

  // Insert new element
  memcpy(ptr, new_element, width);
  return ptr;
}
