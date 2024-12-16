#include <flapjack_io.h>
#include <termio.h>
#include <unistd.h>
#include <error_print.h>
#include <errno.h>
#include <ctype.h>
#include <flapjack_printf.h>

struct termios orig_state;
size_t cursor_pos;
StringArray last_lines;
String* current_line;
size_t row;

void init_terminal_prompt()
{
    last_lines = init_string_array();
    row = 0;
}

void disable_raw_mode()
{
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_state) == -1)
    {
        print_error("Unable to leave raw mode\r\n");
        exit(1);    
    }
}

// https://viewsourcecode.org/snaptoken/kilo/ for entering raw mode
// and handling user input in raw mode
void enable_raw_mode()
{
    if(tcgetattr(STDIN_FILENO, &orig_state) == -1)
    {
        print_error("Unable to get terminal attributes\r\n");
        exit(1);
    }
    atexit(disable_raw_mode);
    struct termios raw;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        print_error("Unable to enter raw mode\r\n");
        exit(1);
    }
}


static Key read_key()
{
    int num_read;
    char c;
    while((num_read = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(num_read == -1 && errno != EAGAIN)
        {
            print_error("Unable to read key input");
            exit(1);
        }
    }
    if(c == '\x1b')
    {
        char seq[3];
        if(read(STDIN_FILENO, &seq[0], 1) != 1)
        {
            return (Key){.special = true, .value = KEY_INVALID};
        }
        if(read(STDIN_FILENO, &seq[1], 1) != 1)
        {
            return (Key){.special = true, .value = KEY_INVALID};
        }
        if(seq[0] == '[')
        {
            if(seq[1] >= '0' && seq[1] <= '9')
            {
                if(read(STDIN_FILENO, &seq[2], 1) != 1)
                {
                    return (Key){.special = true, .value = KEY_INVALID};
                }
                if(seq[2] == '~')
                {
                    switch(seq[1])
                    {
                        case '3':
                        {
                            return (Key){.special = true, .value = KEY_DELETE};
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
                        return (Key){.special = true, .value = KEY_UP};
                    }
                    case 'B':
                    {
                        return (Key){.special = true, .value = KEY_DOWN};
                    }
                    case 'C':
                    {
                        return (Key){.special = true, .value = KEY_RIGHT};
                    }
                    case 'D':
                    {
                        return (Key){.special = true, .value = KEY_LEFT};
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
    return (Key){.special = false, .value = 0};
}

String* get_line(const char* prompt)
{
    current_line = get_string("", 0);
    cursor_pos = 0;
    flapjack_printf("\33[2K\r", 5);
    flapjack_printf("%s >> ", prompt);
    while(true)
    {
        Key next_key = read_key();
        if(next_key.special)
        {
            switch(next_key.value)
            {
                case KEY_DELETE:
                {
                    if(cursor_pos < current_line->len)
                    {
                        current_line = remove_str(current_line, cursor_pos);
                    }
                    row = last_lines.len;
                    break;
                }
                case KEY_BACKSPACE:
                {
                    if(cursor_pos <= current_line->len && cursor_pos > 0)
                    {
                        current_line = remove_str(current_line, cursor_pos - 1);
                        cursor_pos--;
                    }
                    row = last_lines.len;
                    break;
                }
                case KEY_LEFT:
                {
                    if(cursor_pos > 0)
                    {
                        cursor_pos--;
                    }
                    break;
                }
                case KEY_RIGHT:
                {
                    if(cursor_pos < current_line->len)
                    {
                        cursor_pos++;
                    }
                    break;
                }
                case KEY_UP:
                {
                    if(row > 0 && last_lines.len > 0)
                    {
                        row--;
                        current_line = last_lines.elements[row];
                        cursor_pos = current_line->len;
                    }
                    break;
                }
                case KEY_DOWN:
                {
                    if(last_lines.len != 0 && row < last_lines.len - 1)
                    {
                        row++;
                        current_line = last_lines.elements[row];
                        cursor_pos = current_line->len;
                    }
                    break;
                }
                case KEY_NEWLINE:
                {
                    flapjack_printf("\r\n", 2);
                    String* res = current_line;
                    mark_string_global(res);
                    string_array_add_string(&last_lines, res);
                    row = last_lines.len;
                    return res;
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
           current_line = insert_str(current_line, next_key.value, cursor_pos);
           row = last_lines.len;
           cursor_pos++;
        }
        flapjack_printf("\33[2K\r", 5);
        flapjack_printf("%s >> %s", prompt, current_line->msg);
        // https://cloudaffle.com/series/customizing-the-prompt/moving-the-cursor/ for ansi codes for moving the cursor
        flapjack_printf("\e[1C");
        flapjack_printf("\e[%zuD", current_line->len - cursor_pos + 1);
    }
}
