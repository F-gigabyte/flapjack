#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <flapjack_string.h>
#include <flapjack_printf.h>
#include <flapjack_mem.h>
#include <flapjack_parse.h>
#include <flapjack_io.h>
#include <error_print.h>
#include <stdio.h>
#include <sys/stat.h>
#include <flapjack_commands.h>

String* current_dir;

void run_cmdline()
{
    init_terminal_prompt();
    StringArray lines = init_string_array();
    while(true)
    {
        String* line = get_line(current_dir->msg);
        add_string_reference(line);
        string_array_add_string(&lines, line);
        parse_varelse(&current_dir, &lines, lines.len - 1);
    }
    dest_string_array(&lines);
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
        fclose(file);
        exit(1);
    }
    char* buffer = realloc_array(NULL, file_stat.st_size, sizeof(char));
    if(fread(buffer, sizeof(char), file_stat.st_size, file) != file_stat.st_size)
    {
        print_error("Error reading file %s\r\n", name);
        free(buffer);
        fclose(file);
        exit(1);
    }
    fclose(file);
    StringArray lines = init_string_array();
    size_t pos = 0;
    for(size_t i = 0; i < file_stat.st_size; i++)
    {
        if(buffer[i] == '\n')
        {
            if(i - pos > 0)
            {
                String* line = get_string(&buffer[pos], i - pos);
                add_string_reference(line);
                string_array_add_string(&lines, line);
            }
            pos = i + 1;
        }
    }
    if(file_stat.st_size - pos > 0)
    {
        String* line = get_string(&buffer[pos], file_stat.st_size - pos);
        add_string_reference(line);
        string_array_add_string(&lines, line);
    }
    free(buffer);
    parse_varelse(&current_dir, &lines, 0);
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
    update_current_dir(&current_dir); 
    enable_raw_mode();
    init_parser();
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
