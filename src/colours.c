#include <colours.h>

#include <stdio.h>

#define ASCII_ESC 27

void print_esc_code(const char* code)
{
    printf("%c%s", ASCII_ESC, code);
}

void set_text_colour(char colour, bool bright)
{
    print_esc_code("[");
    if(bright)
    {
        printf("%d;1m", colour + 30);
    }
    else
    {
        printf("%dm", colour + 30);
    }
}

void reset_text_colour()
{
    print_esc_code("[0m");
}

