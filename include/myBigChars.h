#ifndef MYBIGCHARS_H
#define MYBIGCHARS_H
#include "myTerm.h"
int bc_box(int top, int left, int height, int width);
int bc_printbigchar(char ch, int row, int col, enum colors fg, enum colors bg);
#endif
