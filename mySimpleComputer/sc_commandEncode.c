#include "mySimpleComputer.h"

int
sc_commandEncode (int sign, int command, int operand, int *value)
{
  int v;

  if (value == NULL)
    {
      return -1;
    }

  if (!((sign == 0) || (sign == 1)))
    {
      return -1;
    }

  if (sc_commandValidate (command) != 0)
    {
      return -1;
    }

  if (operand < 0 || operand > 0x7F)
    {
      return -1;
    }

  v = 0;
  v |= (sign & 0x1) << 14;
  v |= (command & 0x7F) << 7;
  v |= (operand & 0x7F);

  if (v < 0 || v > SC_VALUE_MAX)
    {
      return -1;
    }

  *value = v;
  return 0;
}
