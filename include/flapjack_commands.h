#ifndef FLAPJACK_COMMANDS_H
#define FLAPJACK_COMMANDS_H

#include <string>
#include <vector>
#include <terminal_streams.h>
#include <flapjack_io.h>

std::string update_current_dir(const std::string& current_dir);
int dir_cmd(TerminalIO& terminal, const std::string& current_dir, const std::vector<std::string>& args);
int cd_cmd(TerminalIO& terminal, std::string& current_dir, const std::vector<std::string>& args);
int pwd_cmd(TerminalIO& terminal, const std::string& current_dir, const std::vector<std::string>& args);
int exec_process(TerminalIO& terminal, const std::vector<std::string>& args, const TerminalStream& streams);

#endif
