#include <stdio.h>

#include "mySimpleComputer.h"

static int memory[SC_MEMORY_SIZE];
static int reg_flags;
static int reg_accumulator;
static int reg_icounter;

static int
is_valid_address (int address)
{
  return (address >= 0) && (address < SC_MEMORY_SIZE);
}

static int
is_valid_value (int value)
{
  return (value >= 0) && (value <= SC_VALUE_MAX);
}

/* ---------------- memory ---------------- */

int
sc_memoryInit (void)
{
  for (int i = 0; i < SC_MEMORY_SIZE; i++)
    {
      memory[i] = 0;
    }
  return 0;
}

int
sc_memorySet (int address, int value)
{
  if (!is_valid_address (address) || !is_valid_value (value))
    {
      return -1;
    }

  memory[address] = value;
  return 0;
}

int
sc_memoryGet (int address, int *value)
{
  if (!is_valid_address (address) || value == NULL)
    {
      return -1;
    }

  *value = memory[address];
  return 0;
}

int
sc_memorySave (char *filename)
{
  FILE *f;
  size_t written;

  if (filename == NULL)
    {
      return -1;
    }

  f = fopen (filename, "wb");
  if (f == NULL)
    {
      return -1;
    }

  written = fwrite (memory, sizeof (int), SC_MEMORY_SIZE, f);
  if (written != SC_MEMORY_SIZE)
    {
      fclose (f);
      return -1;
    }

  if (fclose (f) != 0)
    {
      return -1;
    }

  return 0;
}

int
sc_memoryLoad (char *filename)
{
  FILE *f;
  int tmp[SC_MEMORY_SIZE];
  size_t readn;

  if (filename == NULL)
    {
      return -1;
    }

  f = fopen (filename, "rb");
  if (f == NULL)
    {
      return -1;
    }

  readn = fread (tmp, sizeof (int), SC_MEMORY_SIZE, f);
  if (readn != SC_MEMORY_SIZE)
    {
      fclose (f);
      return -1; /* memory must not change on failure */
    }

  if (fclose (f) != 0)
    {
      return -1;
    }

  for (int i = 0; i < SC_MEMORY_SIZE; i++)
    {
      memory[i] = tmp[i];
    }

  return 0;
}



static int
is_valid_flag_mask (int mask)
{
  int all;

  all = SC_FLAG_OVERFLOW | SC_FLAG_DIVZERO | SC_FLAG_OUTMEM
        | SC_FLAG_BADCOMMAND | SC_FLAG_IGNORECLOCK;

  return (mask != 0) && ((mask & all) == mask);
}

int
sc_regInit (void)
{
  reg_flags = 0;
  return 0;
}

int
sc_regSet (int reg, int value)
{
  if (!is_valid_flag_mask (reg))
    {
      return -1;
    }

  if (value)
    {
      reg_flags |= reg;
    }
  else
    {
      reg_flags &= ~reg;
    }

  return 0;
}

int
sc_regGet (int reg, int *value)
{
  if (!is_valid_flag_mask (reg) || value == NULL)
    {
      return -1;
    }

  *value = (reg_flags & reg) ? 1 : 0;
  return 0;
}



int
sc_accumulatorInit (void)
{
  reg_accumulator = 0;
  return 0;
}

int
sc_accumulatorSet (int value)
{
  if (!is_valid_value (value))
    {
      return -1;
    }

  reg_accumulator = value;
  return 0;
}

int
sc_accumulatorGet (int *value)
{
  if (value == NULL)
    {
      return -1;
    }

  *value = reg_accumulator;
  return 0;
}



int
sc_icounterInit (void)
{
  reg_icounter = 0;
  return 0;
}

int
sc_icounterSet (int value)
{
  if (value < 0 || value >= SC_MEMORY_SIZE)
    {
      return -1;
    }

  reg_icounter = value;
  return 0;
}

int
sc_icounterGet (int *value)
{
  if (value == NULL)
    {
      return -1;
    }

  *value = reg_icounter;
  return 0;
}



static void
print_binary15 (int value)
{
  for (int i = 14; i >= 0; i--)
    {
      putchar (((value >> i) & 1) ? '1' : '0');
      if (i % 4 == 0 && i != 0)
        {
          putchar (' ');
        }
    }
}

void
printCell (int address, enum colors fg, enum colors bg)
{
  int value;
  int sign;
  int cmd;
  int op;

  mt_setfgcolor (fg);
  mt_setbgcolor (bg);

  if (sc_memoryGet (address, &value) != 0)
    {
      printf ("????? ");
      mt_setdefaultcolor ();
      return;
    }

  if (sc_commandDecode (value, &sign, &cmd, &op) == 0)
    {
      printf ("%c%02X%02X ", sign ? '-' : '+', cmd, op);
      mt_setdefaultcolor ();
      return;
    }

  printf ("%04X ", value & 0xFFFF);
  mt_setdefaultcolor ();
}

void
printFlags (void)
{
  putchar ((reg_flags & SC_FLAG_OVERFLOW) ? 'O' : '_');
  putchar ((reg_flags & SC_FLAG_DIVZERO) ? 'Z' : '_');
  putchar ((reg_flags & SC_FLAG_OUTMEM) ? 'M' : '_');
  putchar ((reg_flags & SC_FLAG_BADCOMMAND) ? 'C' : '_');
  putchar ((reg_flags & SC_FLAG_IGNORECLOCK) ? 'I' : '_');
}

void
printDecodedCommand (int value)
{
  printf ("DEC: %d\n", value);
  printf ("OCT: %o\n", value);
  printf ("HEX: %X\n", value);
  printf ("BIN: ");
  print_binary15 (value);
  putchar ('\n');
}

void
printAccumulator (void)
{
  printf ("+%04X", reg_accumulator & 0x7FFF);
}

void
printCounters (void)
{
  printf ("%02d", reg_icounter);
}

void
printTerm (int address, int input)
{
  int value;

  if (sc_memoryGet (address, &value) != 0)
    {
      printf ("%c %02d = ????", input ? '>' : ' ', address);
      return;
    }

  printf ("%c %02d = ", input ? '>' : ' ', address);
  printf ("%c%02X%02X", ((value >> 14) & 1) ? '-' : '+', (value >> 7) & 0x7F,
          value & 0x7F);
}

void
printOperation (void)
{
  int ic;
  int value;
  int sign;
  int cmd;
  int op;

  if (sc_icounterGet (&ic) != 0 || sc_memoryGet (ic, &value) != 0)
    {
      putchar ('!');
      return;
    }

  if (sc_commandDecode (value, &sign, &cmd, &op) != 0)
    {
      putchar ('!');
      return;
    }

  printf ("%c%02X:%02X", sign ? '-' : '+', cmd, op);
}
