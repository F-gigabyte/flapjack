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
    size_t references;
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
String* insert_str(String* text, char c, size_t loc);
String* remove_str(String* text, size_t loc);

void init_string_pool();
void clear_string_pool();

String* get_string(const char* text, size_t len);
void remove_string_reference(String* str);
void add_string_reference(String* str);
void dest_string_pool();
bool strings_equal(const String* a, const String* b);

#endif
