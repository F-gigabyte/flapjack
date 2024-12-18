#include <flapjack_commands.h>

#include <flapjack_printf.h>
#include <error_print.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include<flapjack_mem.h>
#include <flapjack_io.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>

static int perform_dir_cmd(const String* path)
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

int dir_cmd(const String* current_dir, const StringArray* args)
{
    if(args->len == 0)
    {
        int ret = perform_dir_cmd(current_dir);
        return ret;
    }
    else
    {
        int ret = 0;
        for(size_t i = 0; i < args->len; i++)
        {
            flapjack_printf("%s:\r\n", args->elements[i]->msg);
            int path_ret = perform_dir_cmd(args->elements[i]);
            if(path_ret != 0)
            {
                ret = -1;
            }
        }
        return ret; 
    }
}


void update_current_dir(String** current_dir)
{
    char* dir = getcwd(NULL, 0);
    setenv("PWD", dir, true);
    if(*current_dir != NULL)
    {
        setenv("OLDPWD", (*current_dir)->msg, true);
        remove_string_reference(*current_dir);
    }
    *current_dir = get_string(dir, strlen(dir));
    add_string_reference(*current_dir);
    free(dir);
}

int cd_cmd(String** current_dir, const StringArray* args)
{
    if(args->len == 0)
    {
        const char* home = getenv("HOME");
        if(home == NULL)
        {
            print_error("Error getting home directory\r\n");
            return -1;
        }
        int value = chdir(home);
        update_current_dir(current_dir);
        if(value != 0)
        {
            print_error("Error opening directory %s\r\n", home);
        }
        return value;
    }
    else if(args->len == 1)
    {
        int value = chdir(args->elements[0]->msg);
        update_current_dir(current_dir);
        if(value != 0)
        {
            print_error("Error opening directory %s\r\n", args->elements[0]->msg);
        }
        return value;
    }
    else
    {
        print_error("Expected at most 1 argument for '@'. Got %zu\r\n", args->len);
        return -1;
    }
}

int pwd_cmd(const String* current_dir, const StringArray* args)
{
    if(args->len == 0)
    {
        flapjack_printf("%s\r\n", current_dir->msg); 
        return 0;
    }
    else
    {
        print_error("Expected no arguments for 'pwd'. Got %zu\r\n", args->len);
        return -1;
    }
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

static String* get_file_path(FILE* handle)
{
    int file_no = fileno(handle);
    if(file_no == -1)
    {
        return NULL;
    }
    char* proclink = NULL;
    int buffer_size = snprintf(proclink, 0, "/proc/self/fd/%d", file_no);
    if(buffer_size < 0)
    {
        return NULL;
    }
    proclink = realloc_array(proclink, buffer_size, sizeof(char));
    buffer_size = snprintf(proclink, buffer_size, "/proc/self/fd/%d", file_no);
    if(buffer_size < 0)
    {
        free(proclink);
    }
    char* res = realpath(proclink, NULL);
    free(proclink);
    if(res == NULL)
    {
        return NULL;
    }
    String* res_str = get_string(res, strlen(res));
    free(res);
    return res_str;
}

int exec_process(const StringArray* args, const String* stdin_path, const String* stdout_path, bool stdout_write)
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
                FILE* stdin_res;
                if(stdin_path != NULL)
                {
                    String* current_stdin = get_file_path(stdin);
                    if(current_stdin == NULL || stdin_path != current_stdin)
                    {
                        stdin_res = freopen(stdin_path->msg, "r", stdin);    
                    }
                }
                FILE* stdout_res = NULL;
                if(stdout_path != NULL)
                {
                    String* current_stdout = get_file_path(stdout);
                    if(stdout_write)
                    {
                        if(current_stdout == NULL || stdout_path != current_stdout)
                        {
                            stdout_res = freopen(stdout_path->msg, "w", stdout);    
                        }
                    }
                    else
                    {
                        if(current_stdout == NULL || stdout_path != current_stdout)
                        {
                            stdout_res = freopen(stdout_path->msg, "a", stdout);    
                        }
                    }
                }
                if((stdin_path != NULL && stdin_res == NULL) || (stdout_path != NULL && stdout_res == NULL))
                {
                    print_error("Unable to redirect child stdin and stdout\r\n");
                    fclose(stdin);
                    fclose(stdout);
                    _exit(1);
                }
                // child, call exec
                int res = execve(paths.elements[i]->msg, arguments, __environ);
                fclose(stdin);
                fclose(stdout);
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
    return -1;
}
