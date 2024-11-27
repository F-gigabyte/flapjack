#ifndef FLAPJACK_STRING_H
#define FLAPJACK_STRING_H

#include <stdlib.h>
#include <stdbool.h>
#include <string_hash.h>

typedef struct
{
    size_t len;
    size_t hash_val;
    bool tombstone;
    char msg[];
} String;

typedef struct
{
    size_t len;
    size_t capacity;
    String** elements;
} StringArray;

StringArray init_string_array();
void string_array_add_string(StringArray* array, String* str);
void dest_string_array(StringArray* array);
String* concat_str(String* a, String* b);

void init_string_pool();

String* get_string(const char* text, size_t len);
void remove_string(String* text);
void dest_string_pool();

#endif
