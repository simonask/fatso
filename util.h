#pragma once
#ifndef FATSO_UTIL_H_INCLUDED
#define FATSO_UTIL_H_INCLUDED

#include <stddef.h> // size_t
#include <sys/types.h> // pid_t
#include <unistd.h> // ssize_t
#include <stdarg.h> // va_list

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

unsigned int
fatso_get_number_of_cpu_cores();

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

#define FATSO_ARRAY(TYPE) struct { TYPE* data; size_t size; }

struct fatso_kv_pair {
  char* key;
  char* value;
};

typedef FATSO_ARRAY(struct fatso_kv_pair) fatso_dictionary_t;

void
fatso_dictionary_init(fatso_dictionary_t* dict);

void
fatso_dictionary_destroy(fatso_dictionary_t* dict);

const char*
fatso_dictionary_get(const fatso_dictionary_t* dict, const char* key);

const struct fatso_kv_pair*
fatso_dictionary_set(fatso_dictionary_t* dict, const char* key, const char* value);

const struct fatso_kv_pair*
fatso_dictionary_insert(fatso_dictionary_t* dict, const struct fatso_kv_pair* pair);

void
fatso_dictionary_merge(fatso_dictionary_t* dict, const fatso_dictionary_t* other);

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

#define fatso_push_back(data, size, new_element, element_size) \
  fatso_push_back_((void**)(data), size, new_element, element_size)

#define fatso_push_back_v(array, new_element) \
  fatso_push_back(&((array)->data), &((array)->size), (new_element), sizeof(*new_element))

void*
fatso_append_(void** inout_data, size_t* inout_num_elements, const void* new_elements, size_t element_size, size_t num_elements);

#define fatso_append(data, size, elements, element_size, num_elements) \
  fatso_append_((void**)(data), size, elements, element_size, num_elements)

#define fatso_append_v(array, elements, num_elements) \
  fatso_append(&((array)->data), &((array)->size), (elements), sizeof(*(array)->data), num_elements)

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


typedef FATSO_ARRAY(char) fatso_strbuf_t;
void fatso_strbuf_init(fatso_strbuf_t*);
void fatso_strbuf_destroy(fatso_strbuf_t*);
void fatso_strbuf_printf(fatso_strbuf_t*, const char* fmt, ...);
void fatso_strbuf_vprintf(fatso_strbuf_t*, const char* fmt, va_list ap);
void fatso_strbuf_append(fatso_strbuf_t*, const char* string, size_t len);
char* fatso_strbuf_strdup(const fatso_strbuf_t*);
char* fatso_strbuf_strndup(const fatso_strbuf_t*, size_t len);


struct fatso_process;
struct fatso_process_callbacks {
  void(*on_stdout)(struct fatso_process*, const void* buffer, size_t len);
  void(*on_stderr)(struct fatso_process*, const void* buffer, size_t len);
};

struct fatso_process*
fatso_process_new(const char* path, const char *const argv[], const struct fatso_process_callbacks* callbacks, void* userdata);

void
fatso_process_free(struct fatso_process*);

void*
fatso_process_userdata(struct fatso_process*);

void
fatso_process_start(struct fatso_process*);

int
fatso_process_wait(struct fatso_process*);

int
fatso_process_wait_all(struct fatso_process**, int* out_statuses, size_t n);

int
fatso_system(const char* command);

int
fatso_system_with_callbacks(const char* command, const struct fatso_process_callbacks* callbacks);

int
fatso_system_with_capture(const char* command, char** output, size_t* output_length);

int
fatso_system_with_split_capture(const char* command, char** stdout_output, size_t* stdout_length, char** stderr_output, size_t* stderr_length);

ssize_t
fatso_process_write(struct fatso_process*, const void* buffer, size_t len);

int
fatso_process_kill(struct fatso_process*, pid_t sig);



#ifdef __cplusplus
}
#endif

#endif // FATSO_UTIL_H_INCLUDED
