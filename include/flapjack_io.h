#ifndef FLAPJACK_IO_H
#define FLAPJACK_IO_H

#include <stdbool.h>
#include <flapjack_string.h>

void enable_raw_mode();
void disable_raw_mode();
String* get_line(const char* prompt);
void init_terminal_prompt();

typedef enum: char
{
    KEY_INVALID = 0,
    KEY_UP = 1,
    KEY_DOWN = 2,
    KEY_LEFT = 3,
    KEY_RIGHT = 4,
    KEY_BACKSPACE = 5,
    KEY_DELETE = 6,
    KEY_NEWLINE = 7,
    KEY_TAB = 8,
} KeyValues;


typedef struct
{
    bool special;
    char value;
} Key;

#endif
