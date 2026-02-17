#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mySimpleComputer.h"

static void
print_memory_dump (void)
{
  for (int i = 0; i < SC_MEMORY_SIZE; i++)
    {
      printCell (i);
      if ((i + 1) % 10 == 0)
        {
          putchar ('\n');
        }
    }
}

int
main (void)
{
  int rc;
  int value;
  int encoded;
  int sign;
  int cmd;
  int op;

  srand ((unsigned)time (NULL));

  /* a */
  sc_memoryInit ();
  sc_accumulatorInit ();
  sc_icounterInit ();
  sc_regInit ();

  /* b */
  for (int i = 0; i < 20; i++)
    {
      int addr = rand () % SC_MEMORY_SIZE;
      int s = rand () % 2;
      int c = rand () % 0x80;
      int o = rand () % 0x80;

      if (sc_commandEncode (s, c, o, &encoded) == 0)
        {
          sc_memorySet (addr, encoded);
        }
    }

  puts ("Memory dump (decoded, 10 per line):");
  print_memory_dump ();

  /* c */
  rc = sc_memorySet (0, SC_VALUE_MAX + 1);
  printf ("Try set invalid memory value -> rc=%d\n", rc);

  /* d */
  sc_regSet (SC_FLAG_OVERFLOW, 1);
  sc_regSet (SC_FLAG_DIVZERO, 1);
  sc_regSet (SC_FLAG_BADCOMMAND, 1);

  puts ("Flags:");
  printFlags ();

  /* e */
  rc = sc_regSet (0x20, 1);
  printf ("Try set invalid flag mask -> rc=%d\n", rc);

  /* f */
  sc_accumulatorSet (1234);
  printAccumulator ();

  /* g */
  rc = sc_accumulatorSet (SC_VALUE_MAX + 10);
  printf ("Try set invalid accumulator -> rc=%d\n", rc);

  /* h */
  sc_icounterSet (10);
  printCounters ();

  /* i */
  rc = sc_icounterSet (SC_MEMORY_SIZE);
  printf ("Try set invalid IC -> rc=%d\n", rc);

  /* j */
  sc_memoryGet (10, &value);
  printf ("Decode memory[10]=%d:\n", value);
  printDecodedCommand (value);

  sc_accumulatorGet (&value);
  printf ("Decode accumulator=%d:\n", value);
  printDecodedCommand (value);

  /* k */
  rc = sc_commandEncode (0, 0x0A, 0x1F, &encoded);
  printf ("Encode command (sign=0, cmd=0x0A, op=0x1F) -> rc=%d, value=%d\n",
          rc, encoded);
  printDecodedCommand (encoded);

  rc = sc_commandDecode (encoded, &sign, &cmd, &op);
  printf ("Decoded back: sign=%d cmd=0x%X op=0x%X (rc=%d)\n", sign, cmd, op,
          rc);

  return 0;
}
