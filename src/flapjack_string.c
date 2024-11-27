#include <flapjack_string.h>
#include <flapjack_mem.h>
#include <string.h>
#include <stdio.h>

#define LOAD_FACTOR 0.8

StringArray pool;

void init_string_pool()
{
    pool = (StringArray){.len = 0, .capacity = 0, .elements = NULL};
}

void dest_string_pool()
{
    for(size_t i = 0; i < pool.capacity; i++)
    {
        if(pool.elements[i] != NULL)
        {
            free(pool.elements[i]);
        }
    }
    free(pool.elements);
    pool = (StringArray){.len = 0, .capacity = 0, .elements = NULL};
}

String* take_str(const char* text, size_t len)
{
    String* res = malloc(sizeof(String) + sizeof(char) * (len + 1));
    if(res == NULL)
    {
        printf("Error: Out of memory\n");
        exit(1);
    }
    res->len = len;
    res->hash_val = hash_string(text, len);
    res->tombstone = false;
    memcpy(res->msg, text, len * sizeof(char));
    res->msg[res->len] = 0;
    return res;
}

size_t find_pool_array_insert_slot(String** array, size_t capacity, String* item)
{
    for(size_t i = 0; true; i++)
    {
        size_t index = (item->hash_val + i) % capacity;
        if(array[index] == NULL || array[index]->tombstone)
        {
            return index;
        }
    }
}

bool same_str(const char* a, const char* b, size_t len_a, size_t len_b)
{
    if(len_a != len_b)
    {
        return false;
    }
    for(size_t i = 0; i < len_a; i++)
    {
        if(a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

bool lookup_string(const char* text, size_t len, size_t* index)
{
    size_t hash_val = hash_string(text, len);
    for(size_t i = 0; true; i++)
    {
        size_t hash_index = (hash_val + i) % pool.capacity;
        if(pool.elements[hash_index] == NULL)
        {
            return false;
        }
        else if(same_str(text, pool.elements[hash_index]->msg, len, pool.elements[hash_index]->len))
        {
            *index = hash_index;
            return true;
        }
    }
}

void resize_pool()
{
    size_t new_cap = get_new_array_capacity(pool.len + 1, pool.capacity);
    size_t new_array_size = new_cap * sizeof(String*);
    String** next_array = malloc(new_array_size);
    memset(next_array, 0, new_array_size);
    if(next_array == NULL)
    {
        printf("Error: Out of memory\n");
        exit(1);
    }
    for(size_t i = 0; i < pool.capacity; i++)
    {
        if(pool.elements[i] != NULL)
        {
            if(!pool.elements[i]->tombstone)
            {
                next_array[find_pool_array_insert_slot(next_array, new_cap, pool.elements[i])] = pool.elements[i];
            }
            else if(pool.elements[i]->tombstone)
            {
                free(pool.elements[i]);
            }    
        }
    }
    pool.capacity = new_cap;
    free(pool.elements);
    pool.elements = next_array;
}

String* get_string(const char* text, size_t len)
{
    if(LOAD_FACTOR * pool.capacity <= (pool.len + 1))
    {
        resize_pool();
    }
    size_t index;
    if(lookup_string(text, len, &index))
    {
        return pool.elements[index];
    }
    else
    {
        String* value = take_str(text, len);
        size_t index = find_pool_array_insert_slot(pool.elements, pool.capacity, value);
        if(pool.elements[index] != NULL)
        {
            free(pool.elements[index]);
        }
        pool.elements[index] = value;
        pool.len++;
        return value;
    }
}

void dest_string(String* text)
{
    text->tombstone = true;
    pool.len--;
}

void resize_array(StringArray* array, size_t len)
{
    size_t new_cap = get_new_array_capacity(len, array->capacity);
    array->elements = realloc_array(array->elements, new_cap, sizeof(String));
    array->capacity = new_cap;
    if(array->elements == NULL)
    {
        printf("Error: Out of memory\n");
        exit(1);
    }
}

StringArray init_string_array()
{
    return (StringArray){.elements = NULL, .capacity = 0, .len = 0};
}

void string_array_add_string(StringArray* array, String* str)
{
    if(array->len >= array->capacity)
    {
        resize_array(array, array->len + 1);
    }
    array->elements[array->len] = str;
    array->len++;
}

void dest_string_array(StringArray* array)
{
    array->elements = realloc_array(array->elements, 0, sizeof(String));
    array->len = 0;
    array->capacity = 0;
}

String* concat_str(String* a, String* b)
{
    size_t combined_len = a->len + b->len;
    char* combined = malloc(sizeof(char) * (combined_len));
    if(combined == NULL)
    {
        printf("Error: Out of memory\n");
        exit(1);
    }
    memcpy(combined, a->msg, a->len * sizeof(char));
    memcpy(combined + a->len, b->msg, b->len * sizeof(char));
    String* res = get_string(combined, combined_len);
    free(combined);
    return res;
}
