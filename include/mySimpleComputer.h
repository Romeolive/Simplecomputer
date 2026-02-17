#ifndef MY_SIMPLE_COMPUTER_H
#define MY_SIMPLE_COMPUTER_H

#include <stdio.h>

#define SC_MEMORY_SIZE 128
#define SC_VALUE_MAX 0x7FFF

#define SC_FLAG_OVERFLOW 0x01
#define SC_FLAG_DIVZERO 0x02
#define SC_FLAG_OUTMEM 0x04
#define SC_FLAG_BADCOMMAND 0x08
#define SC_FLAG_IGNORECLOCK 0x10

int sc_memoryInit (void);
int sc_memorySet (int address, int value);
int sc_memoryGet (int address, int *value);
int sc_memorySave (char *filename);
int sc_memoryLoad (char *filename);

int sc_regInit (void);
int sc_regSet (int reg, int value);
int sc_regGet (int reg, int *value);

int sc_accumulatorInit (void);
int sc_accumulatorSet (int value);
int sc_accumulatorGet (int *value);

int sc_icounterInit (void);
int sc_icounterSet (int value);
int sc_icounterGet (int *value);

int sc_commandEncode (int sign, int command, int operand, int *value);
int sc_commandDecode (int value, int *sign, int *command, int *operand);
int sc_commandValidate (int command);

void printCell (int address);
void printFlags (void);
void printDecodedCommand (int value);
void printAccumulator (void);
void printCounters (void);

#endif
