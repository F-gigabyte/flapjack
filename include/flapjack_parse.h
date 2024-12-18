#ifndef FLAPJACK_PARSE_H
#define FLAPJACK_PARSE_H

#include <flapjack_string.h>

void init_parser();
void parse_varelse(String** current_dir, StringArray* lines, size_t ip);

#endif
