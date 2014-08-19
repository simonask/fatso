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



#ifdef __cplusplus
}
#endif

#endif // FATSO_UTIL_H_INCLUDED
