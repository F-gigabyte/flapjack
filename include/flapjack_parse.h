#ifndef FLAPJACK_PARSE_H
#define FLAPJACK_PARSE_H

#include <string>
#include <vector>
#include <cstddef>
#include <array>
#include <flapjack_io.h>
#include <terminal_streams.h>

#define NUM_REGISTERS 10

class VarelseParser
{
public:
    VarelseParser();
    void parse(TerminalIO& terminal, std::string& current_dir, const std::vector<std::string>& lines, std::size_t ip);
private:
    std::vector<std::string> split_line(const std::string& text);
    bool get_reg_arg(const std::string& index, size_t& arg);
    bool get_command_args(const std::vector<std::string>& line, std::vector<std::string>& args);
    TerminalStream streams;
    std::array<std::string, NUM_REGISTERS> registers;
    std::vector<std::string> stack;
    bool background;
};
#undef NUM_REGISTERS

#endif
