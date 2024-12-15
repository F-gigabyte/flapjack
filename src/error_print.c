#include <error_print.h>
#include <colours.h>
#include <stdarg.h>
#include <unistd.h>
#include <flapjack_mem.h>
#include <flapjack_printf.h>

void print_error(const char* text, ...)
{
    set_text_colour(RED, true);
    va_list args;
    va_start(args, text);
    flapjack_vprintf(text, args);
    va_end(args);
    reset_text_colour();
}
