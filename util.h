#pragma once
#ifndef FATSO_UTIL_H_INCLUDED
#define FATSO_UTIL_H_INCLUDED

#include <stddef.h> // size_t

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

#ifdef __cplusplus
}
#endif

#endif // FATSO_UTIL_H_INCLUDED
