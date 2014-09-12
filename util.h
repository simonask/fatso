#pragma once
#ifndef FATSO_UTIL_H_INCLUDED
#define FATSO_UTIL_H_INCLUDED

#include <stddef.h> // size_t
#include <signal.h> // pid_t
#include <unistd.h> // ssize_t

#ifdef __cplusplus
extern "C" {
#else
  #include <stdbool.h>
#endif

#ifndef NDEBUG
void debugf_(const char* fmt, ...);
#define debugf(...) debugf_(__VA_ARGS__)
#else
#define debugf(...)
#endif

const char*
fatso_get_homedir();

bool
fatso_directory_exists(const char* path);

bool
fatso_file_exists(const char* path);

int
fatso_mkdir_p(const char* path);

int
fatso_run(const char* command);

int
fatso_download(const char* target_path, const char* uri);

/*
  fatso_lower_bound is identical to bsearch, except it returns a pointer to the first element that's *not* less than key,
  instead of NULL when the key is not found.
*/
void*
fatso_lower_bound(const void* key, void* base, size_t nel, size_t width, int(*compare)(const void*, const void*));

void*
fatso_lower_bound_cmp(const void* key, void* base, size_t nel, size_t width, int(*compare)(const void*, const void*), int* out_found_comparison_result);

void*
fatso_bsearch(const void* key, void* base, size_t nel, size_t width, int(*compare)(const void*, const void*));

#define fatso_bsearch_v(key, array, compare) \
  fatso_bsearch(key, (array)->data, (array)->size, sizeof(*((array)->data)), compare)

void*
fatso_push_back_(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t element_size);

#define FATSO_ARRAY(TYPE) struct { TYPE* data; size_t size; }

#define fatso_push_back(data, size, new_element, element_size) \
  fatso_push_back_((void**)(data), size, new_element, element_size)

#define fatso_push_back_v(array, new_element) \
  fatso_push_back(&((array)->data), &((array)->size), (new_element), sizeof(*new_element))

int
fatso_erase(void** inout_data, size_t* inout_num_elements, size_t idx, size_t width);

#define fatso_erase_v(array, idx) \
  fatso_erase(&((array)->data), &((array)->size), idx, sizeof(*((array)->data)))

void*
fatso_multiset_insert(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t width, int(*compare)(const void*, const void*));

#define fatso_multiset_insert_v(array, new_element, compare) \
  fatso_multiset_insert((void**)&((array)->data), &((array)->size), (new_element), sizeof(*new_element), compare)

void*
fatso_set_insert(void** inout_data, size_t* inout_num_elements, const void* new_element, size_t width, int(*compare)(const void*, const void*));

#define fatso_set_insert_v(array, new_element, compare) \
  fatso_set_insert((void**)&((array)->data), &((array)->size), (new_element), sizeof(*new_element), compare)


struct fatso_process;
struct fatso_process_callbacks {
  void(*on_stdout)(struct fatso_process*, const char* buffer, size_t len);
  void(*on_stderr)(struct fatso_process*, const char* buffer, size_t len);
};

struct fatso_process*
fatso_process_new(const char* path, char *const argv[], const struct fatso_process_callbacks* callbacks, void* userdata);

void
fatso_process_free(struct fatso_process*);

void*
fatso_process_userdata(struct fatso_process*);

void
fatso_process_start(struct fatso_process*);

int
fatso_process_wait(struct fatso_process*);

ssize_t
fatso_process_write(struct fatso_process*, const void* buffer, size_t len);

int
fatso_process_kill(struct fatso_process*, pid_t sig);



#ifdef __cplusplus
}
#endif

#endif // FATSO_UTIL_H_INCLUDED
