#include <flapjack_commands.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>
#include <unistd.h>
#include <flapjack_mem.h>
#include <cstdio>

static int perform_dir_cmd(TerminalIO& terminal, const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if(dir == NULL)
    {
        terminal.print_error("Error opening directory %s\r\n", path.c_str());
        return 1;
    }
    struct dirent* entry = readdir(dir);
    while(entry != NULL)
    {
        if(std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
        {
            terminal.print("%s\r\n", entry->d_name);
        }
        entry = readdir(dir);
    }
    terminal.print("\r\n");
    closedir(dir);
    return 0;
}

int dir_cmd(TerminalIO& terminal, const std::string& current_dir, const std::vector<std::string>& args)
{
    if(args.size() == 0)
    {
        int ret = perform_dir_cmd(terminal, current_dir);
        return ret;
    }
    else
    {
        int ret = 0;
        for(size_t i = 0; i < args.size(); i++)
        {
            terminal.print("%s:\r\n", args[i].c_str());
            int path_ret = perform_dir_cmd(terminal, args[i]);
            if(path_ret != 0)
            {
                ret = -1;
            }
        }
        return ret; 
    }
}


std::string update_current_dir(const std::string& current_dir)
{
    char* dir = getcwd(NULL, 0);
    if(dir == NULL)
    {
        // on error, assume we stayed in the original directory
        return current_dir;
    }
    setenv("PWD", dir, true);
    if(current_dir.length() > 0)
    {
        setenv("OLDPWD", current_dir.c_str(), true);
    }
    std::string res = dir;
    free(dir); // dir is malloced
    return res;
}

int cd_cmd(TerminalIO& terminal, std::string& current_dir, const std::vector<std::string>& args)
{
    if(args.size() == 0)
    {
        const char* home = getenv("HOME");
        if(home == NULL)
        {
            terminal.print_error("Error getting home directory\r\n");
            return -1;
        }
        int value = chdir(home);
        current_dir = update_current_dir(current_dir);
        if(value != 0)
        {
            terminal.print_error("Error opening directory %s\r\n", home);
        }
        return value;
    }
    else if(args.size() == 1)
    {
        int value = chdir(args[0].c_str());
        update_current_dir(current_dir);
        if(value != 0)
        {
            terminal.print_error("Error opening directory %s\r\n", args[0].c_str());
        }
        return value;
    }
    else
    {
        terminal.print_error("Expected at most 1 argument for '@'. Got %zu\r\n", args.size());
        return -1;
    }
}

int pwd_cmd(TerminalIO& terminal, const std::string& current_dir, const std::vector<std::string>& args)
{
    if(args.size() == 0)
    {
        terminal.print("%s\r\n", current_dir.c_str()); 
        return 0;
    }
    else
    {
        terminal.print_error("Expected no arguments for 'pwd'. Got %zu\r\n", args.size());
        return -1;
    }
}

char** get_argument_list(const std::vector<std::string>& args)
{
   char** arguments = allocate_array<char*>(args.size() + 1);
   for(size_t i = 0; i < args.size(); i++)
   {
       arguments[i] = allocate_array<char>(args[i].length() + 1);
       std::memcpy(arguments[i], args[i].c_str(), sizeof(char) * args[i].length());
       arguments[i][args[i].length()] = 0;
   }
   arguments[args.size()] = NULL;
   return arguments;
}

std::vector<std::string> parse_env_path()
{
    std::vector<std::string> paths;
    char* path = getenv("PATH");
    if(path == NULL)
    {
        return paths;
    }
    size_t index = 0;
    std::string path_elem;
    while(path[index] != 0)
    {
        if(path[index] == ':')
        {
            if(path_elem.length() > 0)
            {
                paths.emplace_back(path_elem);
            }
            path_elem = "";
        }
        else
        {
            path_elem += path[index];
        }
        index++;
    }
    if(path_elem.length() > 0)
    {
        paths.emplace_back(path_elem);
    }
    return paths;
}

static std::string get_file_path(FILE* handle)
{
    int file_no = fileno(handle);
    if(file_no == -1)
    {
        return NULL;
    }
    std::string proclink;
    int buffer_size = std::snprintf(proclink.data(), 0, "/proc/self/fd/%d", file_no);
    if(buffer_size < 0)
    {
        return "";
    }
    proclink.reserve(buffer_size / sizeof(char));
    buffer_size = std::snprintf(proclink.data(), buffer_size, "/proc/self/fd/%d", file_no);
    if(buffer_size < 0)
    {
        return "";
    }
    char* res = realpath(proclink.c_str(), NULL);
    if(res == NULL)
    {
        return "";
    }
    return res;
}

int exec_process(TerminalIO& terminal, const std::vector<std::string>& args, const TerminalStream& streams)
{
    std::vector<std::string> paths;
    bool has_slash = false;
    for(size_t i = 0; i < args[0].length() && !has_slash; i++)
    {
        if(args[0][i] == '/')
        {
            has_slash = true;
        }
    }
    if(has_slash)
    {
        paths.emplace_back(args[0]);
    }
    else
    {
        paths = parse_env_path();
        for(size_t i = 0; i < paths.size(); i++)
        {
            if(paths[i].back() != '/')
            {
                paths[i] += '/';
            }
            paths[i] += args[0];
        }
    }
    for(size_t i = 0; i < paths.size(); i++)
    {
        if(access(paths[i].c_str(), X_OK) == 0)
        {
            char** arguments = get_argument_list(args);
            int status = 1;
            pid_t p_id = vfork();
            if(p_id == -1)
            {
                terminal.print_error("Unable to create new processes\r\n");
            }
            else if(p_id == 0)
            {
                terminal.disable_raw_mode();
                bool stdin_valid = true;
                if(streams.stdin_path.length() > 0)
                {
                    std::string current_stdin = get_file_path(stdin);
                    if(current_stdin.length() == 0 || streams.stdin_path != current_stdin)
                    {
                        FILE* stdin_res = std::freopen(streams.stdin_path.c_str(), "r", stdin);    
                        stdin_valid = (stdin_res != NULL);
                    }
                }
                bool stdout_valid = true;
                if(streams.stdout_path.length() > 0)
                {
                    std::string current_stdout = get_file_path(stdout);
                    if(streams.stdout_append)
                    {
                        if(current_stdout.length() == 0 || streams.stdout_path != current_stdout)
                        {
                            FILE* stdout_res = freopen(streams.stdout_path.c_str(), "a", stdout);
                            stdout_valid = (stdout_res != NULL);
                        }
                    }
                    else
                    {
                        if(current_stdout.length() == 0 || streams.stdout_path != current_stdout)
                        {
                            FILE* stdout_res = freopen(streams.stdout_path.c_str(), "w", stdout);    
                            stdout_valid = (stdout_res != NULL);
                        }
                    }
                }
                bool stderr_valid = true;
                if(streams.stderr_path.length() > 0)
                {
                    std::string current_stderr = get_file_path(stdout);
                    if(streams.stderr_append)
                    {
                        if(current_stderr.length() == 0 || streams.stderr_path != current_stderr)
                        {
                            FILE* stderr_res = freopen(streams.stderr_path.c_str(), "a", stderr);
                            stderr_valid = (stderr_res != NULL);
                        }
                    }
                    else
                    {
                        if(current_stderr.length() == 0 || streams.stderr_path != current_stderr)
                        {
                            FILE* stderr_res = freopen(streams.stderr_path.c_str(), "w", stderr);    
                            stderr_valid = (stderr_res != NULL);
                        }
                    }
                    
                }
                if(!stdin_valid || !stdout_valid || !stderr_valid)
                {
                    terminal.print_error("Unable to redirect child stdin, stdout and stderr\r\n");
                    fclose(stdin);
                    fclose(stdout);
                    fclose(stderr);
                    _exit(1);
                }
                // child, call exec
                int res = execve(paths[i].c_str(), arguments, __environ);
                fclose(stdin);
                fclose(stdout);
                fclose(stderr);
                _exit(res);
            }
            else
            {
                wait(&status);
                terminal.enable_raw_mode();
            }
            for(size_t i = 0; i < args.size(); i++)
            {
                delete[] arguments[i];
            }
            delete[] arguments;
            return status;
        }
    }
    terminal.print_error("Unknown command '%s'\r\n", args[0].c_str());
    return -1;
}
