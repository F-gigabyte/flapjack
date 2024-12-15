#include <colours.h>

#include <unistd.h>
#include <flapjack_printf.h>

#define ASCII_ESC 27

void print_esc_code(const char* code)
{
    flapjack_printf("%c%s", ASCII_ESC, code);
}

void set_text_colour(char colour, bool bright)
{
    print_esc_code("[");
    if(bright)
    {
        flapjack_printf("%d;1m", colour + 30);
    }
    else
    {
        flapjack_printf("%dm", colour + 30);
    }
}

void reset_text_colour()
{
    print_esc_code("[0m");
}

