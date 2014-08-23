#pragma once
#ifndef FATSO_UTIL_H_INCLUDED
#define FATSO_UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#else
  #include <stdbool.h>
#endif

const char*
fatso_get_homedir();

bool
fatso_directory_exists(const char* path);

bool
fatso_file_exists(const char* path);

int
fatso_run(const char* command);

#define fatso_push_back(data, size, new_element, element_size) \
  (data) = fatso_reallocf((data), ((size) + 1) * (element_size)); \
  memcpy(&(data)[(size)++], &(new_element), element_size)

#define fatso_push_back_v(array, new_element) \
  fatso_push_back((array)->data, (array)->size, (new_element), sizeof(new_element))

#ifdef __cplusplus
}
#endif

#endif // FATSO_UTIL_H_INCLUDED
