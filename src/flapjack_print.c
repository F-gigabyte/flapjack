#include <flapjack_printf.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void flapjack_printf(const char* text, ...)
{
    va_list args;
    va_start(args, text);
    flapjack_vprintf(text, args);
    va_end(args);
}

void flapjack_vprintf(const char* text, va_list args)
{
    char* buffer = NULL;
    int len = vasprintf(&buffer, text, args); 
    if(len == -1 || write(STDOUT_FILENO, buffer, (size_t)len) != len)
    {
       exit(1);
    }
    free(buffer);
}
