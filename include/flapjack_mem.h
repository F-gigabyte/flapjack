#ifndef FLAPJACK_MEM_H
#define FLAPJACK_MEM_H

#include <stdlib.h>

void* realloc_array(void* array, size_t new_size, size_t elem_size);
size_t get_new_array_capacity(size_t size, size_t capacity);

#endif
