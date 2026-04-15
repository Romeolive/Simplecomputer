#include "mySimpleComputer.h"

int
sc_commandValidate (int command)
{
  switch (command)
    {
    case 0x10: /* READ */
    case 0x11: /* WRITE */
    case 0x20: /* LOAD */
    case 0x21: /* STORE */
    case 0x30: /* ADD */
    case 0x31: /* SUB */
    case 0x32: /* DIVIDE */
    case 0x33: /* MUL */
    case 0x40: /* JUMP */
    case 0x41: /* JNEG */
    case 0x42: /* JZ */
    case 0x43: /* HALT */
      return 0;

    default:
      return -1;
    }
}
