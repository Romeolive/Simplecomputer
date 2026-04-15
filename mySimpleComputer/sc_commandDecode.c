#include "mySimpleComputer.h"

int
sc_commandDecode (int value, int *sign, int *command, int *operand)
{

  if (sign == NULL || command == NULL || operand == NULL)
    return -1;

  /* проверка старшего бита */
  if (value & (1 << 15))
    return -1;

  *sign = (value >> 14) & 0x1;
  *command = (value >> 7) & 0x7F;
  *operand = value & 0x7F;

  if (sc_commandValidate (*command) != 0)
    return -1;

  return 0;
}
