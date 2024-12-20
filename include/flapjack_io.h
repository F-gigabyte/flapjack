#ifndef FLAPJACK_IO_H
#define FLAPJACK_IO_H

#include <string>
#include <termio.h>
#include <vector>
#include <cstdio>

enum class TerminalColour
{
    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    PURPLE = 5,
    CYAN = 6,
    LIGHT_GREY = 7,
    DARK_GREY = 8,
    LIGHT_RED = 9,
    LIGHT_GREEN = 10,
    LIGHT_YELLOW = 11,
    LIGHT_BLUE = 12,
    LIGHT_PURPLE = 13,
    LIGHT_CYAN = 14,
    WHITE = 15,
};

struct Key;

class TerminalIO
{
public:
    TerminalIO();
    ~TerminalIO();
    std::string get_line(const std::string& prompt, std::vector<std::string>& lines);
    void print(const char* format, ...);
    void print_error(const char* format, ...);
    void enable_raw_mode();
    void disable_raw_mode();
    void set_text_colour(std::FILE* stream, TerminalColour colour);
    void reset_text_colour(std::FILE* stream);
private:
    Key read_key();
    struct termios original_state;
};

#endif
