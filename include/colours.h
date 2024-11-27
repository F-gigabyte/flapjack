#ifndef FLAPJACK_COLOURS_H
#define FLAPJACK_COLOURS_H

#include <stdbool.h>

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define PURPLE 5
#define CYAN 6
#define WHITE 7

void set_text_colour(char colour, bool bright);
void reset_text_colour();

#endif
