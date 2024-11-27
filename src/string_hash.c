#include <string_hash.h>

size_t hash_string(const char* text, size_t len)
{
    size_t p_change = 257;
    size_t p = 1;
    size_t hash_val = 0;
    for(size_t i = 0; i < len; i++)
    {
        hash_val += text[i] * p;
        p *= p_change;
    }
    return hash_val;
}
