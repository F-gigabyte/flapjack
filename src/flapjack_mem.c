#include <flapjack_mem.h>

#include <stdio.h>

void* realloc_array(void* array, size_t new_size, size_t elem_size)
{
    void* res = realloc(array, new_size * elem_size);
    if(new_size != 0 && res == NULL)
    {
        printf("Error in realloc...\n");
        exit(1);
    }
    return res;
}

size_t get_new_array_capacity(size_t size, size_t capacity)
{
    if(capacity == 0)
    {
        if(size > 8)
        {
            return size;
        }
        else
        {
            return 8;
        }
    }
    capacity *= 2;
    while(capacity < size)
    {
        capacity *= 2;
    }
    return capacity;
}

