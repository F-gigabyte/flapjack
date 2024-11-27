#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <flapjack_string.h>
#include <string.h>
#include <error_print.h>
#include <unistd.h>
#include <sys/wait.h>

StringArray split_line(const char* line)
{
    StringArray arg_array = init_string_array();
    size_t offset = 0;
    size_t i = 0;
    for(; line[i] != 0; i++)
    {
        if(line[i] == ' ')
        {
            if(i - offset > 0)
            {
                String* arg = get_string(&line[offset], i - offset);
                string_array_add_string(&arg_array, arg);
            }
            offset = i + 1;
        }
    }
    if(i - offset > 0)
    {
        String* arg = get_string(&line[offset], i - offset);
        string_array_add_string(&arg_array, arg);
    }
    return arg_array;
}

char* get_user_line()
{
    char* line = NULL;
    size_t line_size = 0;
    ssize_t res = getline(&line, &line_size, stdin);
    if(res == -1)
    {
        free(line);
        printf("Error in getline...\n");
        exit(1);
    }
    line[res - 1] = 0;
    return line;
}

String* current_dir;

void update_current_dir()
{
    char* dir = getcwd(NULL, 0);
    current_dir = get_string(dir, strlen(dir));
    free(dir);
}

int perform_dir_cmd(const String* path)
{
    DIR* dir = opendir(path->msg);
    if(dir == NULL)
    {
        print_error("Error opening directory %s\n", path->msg);
        return 1;
    }
    struct dirent* entry = readdir(dir);
    while(entry != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            printf("%s\n", entry->d_name);
        }
        entry = readdir(dir);
    }
    printf("\n");
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
            printf("%s:\n", args->elements[i]->msg);
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
            print_error("Error getting home directory\n");
            return -1;
        }
        int value = chdir(home);
        update_current_dir();
        if(value != 0)
        {
            print_error("Error opening directory %s\n", home);
        }
        return value;
    }
    else if(args->len == 2)
    {
        int value = chdir(args->elements[1]->msg);
        update_current_dir();
        if(value != 0)
        {
            print_error("Error opening directory %s\n", args->elements[1]->msg);
        }
        return value;
    }
    else
    {
        print_error("Expected at most 2 arguments for cd. Got %zu\n", args->len);
        return -1;
    }
}

void pwd_cmd()
{
    printf("%s\n", current_dir->msg);
}

char** get_argument_list(const StringArray* args)
{
   char** arguments =  malloc(sizeof(char*) * (args->len + 1));
   if(arguments == NULL)
   {
       printf("Error: Out of memory\n");
       exit(1);
   }
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
            pid_t p_id = fork();
            if(p_id == -1)
            {
                print_error("Unable to create new processes");
            }
            else if(p_id == 0)
            {
                // child, call exec
                exit(execv(paths.elements[i]->msg, arguments));
            }
            else
            {
                wait(&status);
            }
            free(arguments);
            dest_string_array(&paths);
            return status;
        }
    }
    print_error("Unknown command '%s'\n", args->elements[0]->msg);
    dest_string_array(&paths);
    return 1;
}

int main()
{
    init_string_pool();
    update_current_dir();
    String* dir_str = get_string("dir", 3);
    String* cd_str = get_string("cd", 2);
    String* exit_str = get_string("exit", 4);
    String* pwd_str = get_string("pwd", 3);
    while(true)
    {
        printf("%s >> ", current_dir->msg);
        char* line = get_user_line();
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
        free(line);
    }
    dest_string_pool();
    return 0;
}
