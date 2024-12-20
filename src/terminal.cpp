#include <terminal.h>
#include <cstdio>
#include <sys/stat.h>
#include <flapjack_commands.h>

Terminal::Terminal(const std::string& call_name) : terminal_io(), parser()
{
    current_dir = update_current_dir(current_dir);
    if(setenv("SHELL", call_name.c_str(), true) != 0)
    {
        terminal_io.print_error("Unable to overwrite 'SHELL' environment variable\r\n");
    }
}

void Terminal::run_cmdline()
{
    while(true)
    {
        std::string line = terminal_io.get_line(current_dir, lines);
        lines.emplace_back(line);
        parser.parse(terminal_io, current_dir, lines, lines.size() - 1);
    }
}

void Terminal::run_file(const std::string& file_name)
{
    std::FILE* file = fopen(file_name.c_str(), "r");
    if(file == NULL)
    {
        terminal_io.print_error("Unable to open file '%s'\r\n", file_name.c_str());
        std::exit(1);
    }
    struct stat file_state;
    if(stat(file_name.c_str(), &file_state) == -1)
    {
        terminal_io.print_error("Unable to get size of file '%s'\r\n", file_name.c_str());
        fclose(file);
        std::exit(1);
    }
    std::string buffer;
    buffer.reserve(file_state.st_size);
    if(std::fread(buffer.data(), sizeof(char), file_state.st_size / sizeof(char), file) != file_state.st_size / sizeof(char))
    {
        terminal_io.print_error("Error reading file '%s'\r\n", file_name.c_str());
        fclose(file);
        std::exit(1);
    }
    std::vector<std::string> lines;
    std::string line = "";
    for(std::size_t i = 0; i < buffer.length(); i++)
    {
        if(buffer[i] == '\n')
        {
            if(line.length() > 0)
            {
                lines.emplace_back(line);
                line = "";    
            }
        }
        else
        {
            line += buffer[i];
        }
    }
    if(line.length() > 0)
    {
        lines.emplace_back(line);
    }
    parser.parse(terminal_io, current_dir, lines, 0);
}
