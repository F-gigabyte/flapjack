#ifndef TERMINAL_H
#define TERMINAL_H

#include <string>
#include <flapjack_io.h>
#include <flapjack_parse.h>
#include <vector>

class Terminal
{
public:
    Terminal();
    void run_cmdline();
    void run_file(const std::string& file_name);
private:
    std::string current_dir;
    TerminalIO terminal_io;
    VarelseParser parser;
    std::vector<std::string> lines;
};

#endif
