#include <error_print.h>
#include <colours.h>
#include <stdio.h>
#include <stdarg.h>

void print_error(const char* text, ...)
{
    set_text_colour(RED, true);
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    reset_text_colour();
}
