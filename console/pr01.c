#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mySimpleComputer.h"

static void print_memory_dump(void) {
  for (int i = 0; i < SC_MEMORY_SIZE; i++) {
    printCell(i, COLOR_WHITE, COLOR_BLACK);
    if ((i + 1) % 10 == 0) {
      putchar('\n');
    }
  }
}

int main(void) {
  int rc;
  int value;
  int encoded;
  int sign;
  int cmd;
  int op;

  srand((unsigned)time(NULL));

  /* a */
  sc_memoryInit();
  sc_accumulatorInit();
  sc_icounterInit();
  sc_regInit();

  /* b */
  for (int i = 0; i < 20; i++) {
    int addr = rand() % SC_MEMORY_SIZE;
    int s = rand() % 2;
    int c = rand() % 0x80;
    int o = rand() % 0x80;

    if (sc_commandEncode(s, c, o, &encoded) == 0) {
      sc_memorySet(addr, encoded);
    }
  }

  puts("Дамп памяти (декодировано, по 10 ячеек в строке):");
  print_memory_dump();

  /* c */
  rc = sc_memorySet(0, SC_VALUE_MAX + 1);
  printf("Попытка записать некорректное значение в память -> rc=%d\n", rc);

  /* d */
  sc_regSet(SC_FLAG_OVERFLOW, 1);
  sc_regSet(SC_FLAG_DIVZERO, 1);
  sc_regSet(SC_FLAG_BADCOMMAND, 1);

  puts("Флаги:");
  printFlags();

  /* e */
  rc = sc_regSet(0x20, 1);
  printf("Попытка установить некорректный флаг -> rc=%d\n", rc);

  /* f */
  sc_accumulatorSet(1234);
  printAccumulator();

  /* g */
  rc = sc_accumulatorSet(SC_VALUE_MAX + 10);
  printf("Попытка установить некорректное значение аккумулятора -> rc=%d\n",
         rc);

  /* h */
  sc_icounterSet(10);
  printCounters();

  /* i */
  rc = sc_icounterSet(SC_MEMORY_SIZE);
  printf("Попытка установить некорректное значение счётчика команд -> rc=%d\n",
         rc);

  /* j */
  sc_memoryGet(10, &value);
  printf("Декодирование значения memory[10]=%d:\n", value);
  printDecodedCommand(value);

  sc_accumulatorGet(&value);
  printf("Декодирование значения аккумулятора=%d:\n", value);
  printDecodedCommand(value);

  /* k */
  rc = sc_commandEncode(0, 0x0A, 0x1F, &encoded);
  printf("Кодирование команды (sign=0, cmd=0x0A, op=0x1F) -> rc=%d, value=%d\n",
         rc, encoded);
  printDecodedCommand(encoded);

  rc = sc_commandDecode(encoded, &sign, &cmd, &op);
  printf("Обратно декодировано: sign=%d cmd=0x%X op=0x%X (rc=%d)\n", sign, cmd,
         op, rc);

  /* k2: пример "отрицательного" слова через sign=1 */
  rc = sc_commandEncode(1, 0x0A, 0x1F, &encoded);
  printf("Кодирование команды со знаком '-' (sign=1, cmd=0x0A, op=0x1F)"
         " -> rc=%d, value=%d\n",
         rc, encoded);

  sc_memorySet(1, encoded);
  printf("Печать memory[1] (ожидаем '-0A1F'): ");
  printCell(1, COLOR_WHITE, COLOR_BLACK);
  putchar('\n');

  return 0;
}
