#ifndef TERMINAL_STREAMS_H
#define TERMINAL_STREAMS_H

#include <string>

struct TerminalStream
{
    std::string stdin_path;
    std::string stdout_path;
    bool stdout_append;
    std::string stderr_path;
    bool stderr_append;
};

#endif
