#ifndef FLAPJACK_COMMANDS_H
#define FLAPJACK_COMMANDS_H

#include <flapjack_string.h>

void update_current_dir(String** current_dir);
int dir_cmd(const String* current_dir, const StringArray* args);
int cd_cmd(String** current_dir, const StringArray* args);
int pwd_cmd(const String* current_dir, const StringArray* args);
int exec_process(const StringArray* args, const String* stdin_path, const String* stdout_path, bool stdout_write);

#endif
