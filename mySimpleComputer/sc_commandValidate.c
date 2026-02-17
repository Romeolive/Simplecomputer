#include "mySimpleComputer.h"
int
sc_commandValidate (int command)
{
  if (command < 0 || command > 0x7F)
    {
      return -1;
    }

  return 0;
}
