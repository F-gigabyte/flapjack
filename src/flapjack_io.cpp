#include <chrono>
#include <flapjack_io.h>
#include <termio.h>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <unistd.h>
#include <thread>

enum KeyValue: char
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
};

struct Key
{
    bool special;
    char value;
};

TerminalIO::TerminalIO()
{
    if(tcgetattr(STDIN_FILENO, &original_state) == -1)
    {
        fprintf(stderr, "Unable to get terminal attributes\r\n");
        exit(1);
    }
    enable_raw_mode();
}

TerminalIO::~TerminalIO()
{
   disable_raw_mode(); 
}

void TerminalIO::set_text_colour(std::FILE* stream, TerminalColour colour)
{
    std::fputs("\x1b[", stream);
    int colour_val = static_cast<int>(colour);
    if(colour_val >= 8)
    {
        // 30 - 8 = 22
        std::fprintf(stream, "%d;1m", colour_val + 22);
    }
    else
    {
        std::fprintf(stream, "%dm", colour_val + 30);
    }
    std::fflush(stdout);
}

void TerminalIO::reset_text_colour(std::FILE* stream)
{
    std::fputs("\x1b[0m", stream);
}

void TerminalIO::print(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    std::vfprintf(stdout, format, args);
    va_end(args);
    std::fflush(stdout);
}

void TerminalIO::print_error(const char* format, ...)
{
    set_text_colour(stderr, TerminalColour::LIGHT_RED);
    std::va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    reset_text_colour(stderr);
    std::fflush(stderr);
}

void TerminalIO::disable_raw_mode()
{
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_state) == -1)
    {
        std::fprintf(stderr, "Unable to leave raw mode\r\n");
        exit(1); 
    }
}

// https://viewsourcecode.org/snaptoken/kilo/ for entering raw mode
// and handling user input in raw mode
void TerminalIO::enable_raw_mode()
{
    struct termios raw = original_state;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        std::fprintf(stderr, "Unable to enter raw mode\r\n");
        exit(1);
    }
}


Key TerminalIO::read_key()
{
    int num_read;
    char c = 0;
    while((num_read = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(num_read == -1 && errno != EAGAIN)
        {
            std::fprintf(stderr, "Unable to read key input");
            exit(1);
        }
        c = 0;
    }
    if(c == '\x1b')
    {
        char seq[3];
        if(read(STDIN_FILENO, &seq[0], 1) != 1)
        {
            return (Key){.special = true, .value = KeyValue::KEY_INVALID};
        }
        if(read(STDIN_FILENO, &seq[1], 1) != 1)
        {
            return (Key){.special = true, .value = KeyValue::KEY_INVALID};
        }
        if(seq[0] == '[')
        {
            if(seq[1] >= '0' && seq[1] <= '9')
            {
                if(read(STDIN_FILENO, &seq[2], 1) != 1)
                {
                    return (Key){.special = true, .value = KeyValue::KEY_INVALID};
                }
                if(seq[2] == '~')
                {
                    switch(seq[1])
                    {
                        case '3':
                        {
                            return (Key){.special = true, .value = KeyValue::KEY_DELETE};
                        }
                    }
                }
            }
            else
            {
                switch(seq[1])
                {
                    case 'A':
                    {
                        return (Key){.special = true, .value = KeyValue::KEY_UP};
                    }
                    case 'B':
                    {
                        return (Key){.special = true, .value = KeyValue::KEY_DOWN};
                    }
                    case 'C':
                    {
                        return (Key){.special = true, .value = KeyValue::KEY_RIGHT};
                    }
                    case 'D':
                    {
                        return (Key){.special = true, .value = KeyValue::KEY_LEFT};
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }
        return (Key){.special = true, .value = KEY_INVALID};
    }
    else if(c == 127)
    {
        return (Key){.special = true, .value = KEY_BACKSPACE};
    }
    else if(c == '\n' || c == '\r')
    {
        return (Key){.special = true, .value = KEY_NEWLINE};
    }
    else if(c == '\t')
    {
        return (Key){.special = true, .value = KEY_TAB};
    }
    else if(!iscntrl(c))
    {
        return (Key){.special = false, .value = c};
    }
    return (Key){.special = true, .value = KeyValue::KEY_INVALID};
}

std::string TerminalIO::get_line(const std::string& prompt, std::vector<std::string>& lines)
{
    std::string current_line = "";
    std::size_t cursor_pos = 0;
    std::size_t row = lines.size();
    print("\33[2K\r");
    print("%s >> ", prompt.c_str());
    while(true)
    {
        Key next_key = read_key();
        if(next_key.special)
        {
            switch(next_key.value)
            {
                case KeyValue::KEY_DELETE:
                {
                    if(cursor_pos < current_line.length())
                    {
                        current_line.erase(cursor_pos, 1);
                    }
                    row = lines.size();
                    break;
                }
                case KeyValue::KEY_BACKSPACE:
                {
                    if(cursor_pos <= current_line.length() && cursor_pos > 0)
                    {
                        current_line = current_line.erase(cursor_pos - 1, 1);
                        cursor_pos--;
                    }
                    row = lines.size();
                    break;
                }
                case KeyValue::KEY_LEFT:
                {
                    if(cursor_pos > 0)
                    {
                        cursor_pos--;
                    }
                    break;
                }
                case KeyValue::KEY_RIGHT:
                {
                    if(cursor_pos < current_line.length())
                    {
                        cursor_pos++;
                    }
                    break;
                }
                case KeyValue::KEY_UP:
                {
                    if(row > 0 && lines.size() > 0)
                    {
                        row--;
                        current_line = lines[row];
                        cursor_pos = current_line.length();
                    }
                    break;
                }
                case KeyValue::KEY_DOWN:
                {
                    if(lines.size() != 0 && row < lines.size() - 1)
                    {
                        row++;
                        current_line = lines[row];
                        cursor_pos = current_line.length();
                    }
                    break;
                }
                case KeyValue::KEY_NEWLINE:
                {
                    print("\r\n");
                    return current_line;
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
           current_line.insert(cursor_pos, 1, next_key.value);
           row = lines.size();
           cursor_pos++;
        }
        print("\33[2K\r");
        print("%s >> %s", prompt.c_str(), current_line.c_str());
        // https://cloudaffle.com/series/customizing-the-prompt/moving-the-cursor/ for ansi codes for moving the cursor
        print("\e[1C");
        print("\e[%zuD", current_line.length() - cursor_pos + 1);
    }
}
