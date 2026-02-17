#include "mySimpleComputer.h"

int
sc_commandDecode (int value, int *sign, int *command, int *operand)
{
  int s;
  int c;
  int o;

  if (sign == NULL || command == NULL || operand == NULL)
    {
      return -1;
    }

  if (value < 0 || value > SC_VALUE_MAX)
    {
      return -1;
    }

  s = (value >> 14) & 0x1;
  c = (value >> 7) & 0x7F;
  o = value & 0x7F;

  if (sc_commandValidate (c) != 0)
    {
      return -1;
    }

  *sign = s;
  *command = c;
  *operand = o;

  return 0;
}
