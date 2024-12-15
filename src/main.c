#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <flapjack_string.h>
#include <flapjack_printf.h>
#include <flapjack_mem.h>
#include <stdio.h>
#include <string.h>
#include <error_print.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

struct termios orig_state;
size_t cursor_pos;

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

StringArray last_lines;
String* current_line;
size_t row;

void disable_raw_mode()
{
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_state) == -1)
    {
        print_error("Unable to leave raw mode\r\n");
        exit(1);
    }
}

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

Key read_key()
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

StringArray split_line(String* line)
{
    StringArray arg_array = init_string_array();
    size_t offset = 0;
    size_t i = 0;
    bool in_quotes = false;
    char start_quote = 0;
    for(; i < line->len; i++)
    {
        if(in_quotes)
        {
            if(line->msg[i] == start_quote)
            {
                in_quotes = false;
                start_quote = 0;
                if(i - offset > 0)
                {
                    String* arg = get_string(&line->msg[offset], i - offset);
                    string_array_add_string(&arg_array, arg);
                }
                offset = i + 1;
            }
        }
        else if(line->msg[i] == ' ' || line->msg[i] == '\'' || line->msg[i] == '\"')
        {
            if(i - offset > 0)
            {
                String* arg = get_string(&line->msg[offset], i - offset);
                string_array_add_string(&arg_array, arg);
            }
            offset = i + 1;
            if(line->msg[i] == '\'')
            {
                in_quotes = true;
                start_quote = '\'';
            }
            else if(line->msg[i] == '\"')
            {
                in_quotes = true;
                start_quote = '\"';
            }
        }
    }
    if(i - offset > 0)
    {
        String* arg = get_string(&line->msg[offset], i - offset);
        string_array_add_string(&arg_array, arg);
    }
    return arg_array;
}

/*
 *  0 h 1 e 2 l 3 l 4 o 5
 *  
 *
 *  insert a 0 -> ahello -> cursor_pos 1
 *  delete a 0 -> ello -> cursor_pos 0
 *  backspace a 1 -> ello -> cursor_pos 0
 *
 */

String* current_dir;

String* get_line()
{
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
                    write(STDOUT_FILENO, "\r\n", 2);
                    String* res = current_line;
                    mark_string_global(res);
                    string_array_add_string(&last_lines, res);
                    cursor_pos = 0;
                    current_line = get_string("", 0);
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
        write(STDOUT_FILENO, "\33[2K\r", 5);
        flapjack_printf("%s >> %s", current_dir->msg, current_line->msg);
        flapjack_printf("\e[1C");
        flapjack_printf("\e[%zuD", current_line->len - cursor_pos + 1);
    }
}

void update_current_dir()
{
    char* dir = getcwd(NULL, 0);
    setenv("PWD", dir, true);
    if(current_dir != NULL)
    {
        setenv("OLDPWD", current_dir->msg, true);
        mark_string_temp(current_dir);
    }
    current_dir = get_string(dir, strlen(dir));
    mark_string_global(current_dir);
    free(dir);
}

int perform_dir_cmd(const String* path)
{
    DIR* dir = opendir(path->msg);
    if(dir == NULL)
    {
        print_error("Error opening directory %s\r\n", path->msg);
        return 1;
    }
    struct dirent* entry = readdir(dir);
    while(entry != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            flapjack_printf("%s\r\n", entry->d_name);
        }
        entry = readdir(dir);
    }
    flapjack_printf("\r\n");
    closedir(dir);
    return 0;
}

int dir_cmd(const StringArray* args)
{
    if(args->len == 1)
    {
        int ret = perform_dir_cmd(current_dir);
        return ret;
    }
    else
    {
        int ret = 0;
        for(size_t i = 1; i < args->len; i++)
        {
            flapjack_printf("%s:\r\n", args->elements[i]->msg);
            int path_ret = perform_dir_cmd(args->elements[i]);
            if(path_ret != 0)
            {
                ret = 1;
            }
        }
        return ret; 
    }
}

int cd_cmd(const StringArray* args)
{
    if(args->len == 1)
    {
        const char* home = getenv("HOME");
        if(home == NULL)
        {
            print_error("Error getting home directory\r\n");
            return -1;
        }
        int value = chdir(home);
        update_current_dir();
        if(value != 0)
        {
            print_error("Error opening directory %s\r\n", home);
        }
        return value;
    }
    else if(args->len == 2)
    {
        int value = chdir(args->elements[1]->msg);
        update_current_dir();
        if(value != 0)
        {
            print_error("Error opening directory %s\r\n", args->elements[1]->msg);
        }
        return value;
    }
    else
    {
        print_error("Expected at most 2 arguments for cd. Got %zu\r\n", args->len);
        return -1;
    }
}

void pwd_cmd()
{
    flapjack_printf("%s\r\n", current_dir->msg);
}

char** get_argument_list(const StringArray* args)
{
   char** arguments = NULL;
   arguments = realloc_array(arguments, args->len + 1, sizeof(char*));
   for(size_t i = 0; i < args->len; i++)
   {
       arguments[i] = args->elements[i]->msg;
   }
   arguments[args->len] = NULL;
   return arguments;
}

StringArray parse_env_path()
{
    StringArray paths = init_string_array();
    char* path = getenv("PATH");
    if(path == NULL)
    {
        return paths;
    }
    size_t index = 0;
    char* path_start = path;
    size_t len = 0;
    while(path[index] != 0)
    {
        if(path[index] == ':')
        {
            if(len > 0)
            {
                String* path_elem = get_string(path_start, len);
                string_array_add_string(&paths, path_elem);
            }
            path_start = &path[index + 1];
            len = 0;
        }
        else
        {
            len++;
        }
        index++;
    }
    return paths;
}

int exec_process(const StringArray* args)
{
    StringArray paths;
    bool has_slash = false;
    for(size_t i = 0; i < args->elements[0]->len && !has_slash; i++)
    {
        if(args->elements[0]->msg[i] == '/')
        {
            has_slash = true;
        }
    }
    if(has_slash)
    {
        paths = init_string_array();
        string_array_add_string(&paths, args->elements[0]);
    }
    else
    {
        paths = parse_env_path();
        String* slash = get_string("/", 1);
        for(size_t i = 0; i < paths.len; i++)
        {
            if(paths.elements[i]->msg[paths.elements[i]->len - 1] != '/')
            {
                paths.elements[i] = concat_str(paths.elements[i], slash);
            }
            paths.elements[i] = concat_str(paths.elements[i], args->elements[0]);
        }
    }
    for(size_t i = 0; i < paths.len; i++)
    {
        if(access(paths.elements[i]->msg, X_OK) == 0)
        {
            char** arguments = get_argument_list(args);
            int status = 1;
            pid_t p_id = vfork();
            if(p_id == -1)
            {
                print_error("Unable to create new processes\r\n");
            }
            else if(p_id == 0)
            {
                disable_raw_mode();
                // child, call exec
                int res = execve(paths.elements[i]->msg, arguments, __environ);
                _exit(res);
            }
            else
            {
                wait(&status);
                enable_raw_mode();
            }
            free(arguments);
            dest_string_array(&paths);
            return status;
        }
    }
    print_error("Unknown command '%s'\r\n", args->elements[0]->msg);
    dest_string_array(&paths);
    return 1;
}

void run_line(String* line)
{
    String* dir_str = get_string("dir", 3);
    String* cd_str = get_string("cd", 2);
    String* exit_str = get_string("exit", 4);
    String* pwd_str = get_string("pwd", 3);
    StringArray args = split_line(line);
    if(args.len > 0)
    {
        if(args.elements[0] == dir_str)
        {
            dir_cmd(&args);
        }
        else if(args.elements[0] == cd_str)
        {
            cd_cmd(&args);
        }
        else if(args.elements[0] == exit_str)
        {
            exit(0);
        }
        else if(args.elements[0] == pwd_str)
        {
            pwd_cmd();
        }
        else
        {
            exec_process(&args);
        }
    }
    dest_string_array(&args);
    clear_string_pool();
}

void run_cmdline()
{
    last_lines = init_string_array();
    row = 0;
    cursor_pos = 0;
    current_line = get_string("", 0);
    mark_string_global(current_line);
    while(true)
    {
        flapjack_printf("%s >> ", current_dir->msg);
        String* line = get_line();
        run_line(line);
    }
}

void run_file(const char* name)
{
    FILE* file = fopen(name, "r");
    if(file == NULL)
    {
        print_error("Unable to open file %s\r\n", name);
        exit(1);
    }
    struct stat file_stat;
    if(stat(name, &file_stat) == -1)
    {
        print_error("Unable to get size of file %s\r\n", name);
        exit(1);
    }
    char* buffer = realloc_array(NULL, file_stat.st_size, sizeof(char));
    if(fread(buffer, sizeof(char), file_stat.st_size, file) != file_stat.st_size)
    {
        print_error("Error reading file %s\r\n", name);
        exit(1);
    }
    fclose(file);
    size_t pos = 0;
    for(size_t i = 0; i < file_stat.st_size; i++)
    {
        if(buffer[i] == '\n')
        {
            if(i - pos > 0)
            {
                run_line(get_string(&buffer[pos], i - pos));
            }
            pos = i + 1;
        }
    }
    if(file_stat.st_size - pos > 0)
    {
        run_line(get_string(&buffer[pos], file_stat.st_size - pos));
    }
}

int main(int argc, const char* argv[])
{
    if(argc != 1 && argc != 2)
    {
        print_error("Expected 1 or 2 arguments\r\n");
        exit(1);
    }
    if(setenv("SHELL", argv[0], true) == -1)
    {
        print_error("Unable to overwrite SHELL variable\r\n");
    }
    init_string_pool();
    update_current_dir();
    enable_raw_mode();
    if(argc == 1)
    {
        run_cmdline();
    }
    else
    {
        run_file(argv[1]);
    }
    dest_string_pool();
    return 0;
}
