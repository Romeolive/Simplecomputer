#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/mySimpleComputer.h"

#define MAX_LINE_LEN 256

static void
trim_left (char **s)
{
  while (**s != '\0' && isspace ((unsigned char)**s))
    {
      (*s)++;
    }
}

static void
trim_right (char *s)
{
  int len = (int)strlen (s);
  while (len > 0 && isspace ((unsigned char)s[len - 1]))
    {
      s[len - 1] = '\0';
      len--;
    }
}

static int
parse_address (const char *s, int *addr)
{
  char *endptr;
  long v;

  if (s == NULL || addr == NULL)
    return -1;

  v = strtol (s, &endptr, 10);
  if (*endptr != '\0')
    return -1;
  if (v < 0 || v >= SC_MEMORY_SIZE)
    return -1;

  *addr = (int)v;
  return 0;
}

static int
parse_operand (const char *s, int *operand)
{
  char *endptr;
  long v;

  if (s == NULL || operand == NULL)
    return -1;

  v = strtol (s, &endptr, 10);
  if (*endptr != '\0')
    return -1;
  if (v < 0 || v > 127)
    return -1;

  *operand = (int)v;
  return 0;
}

static int
parse_data_value (const char *s, int *value)
{
  int sign = 0;
  int mag = 0;

  if (s == NULL || value == NULL)
    return -1;

  if (*s == '+')
    {
      sign = 0;
      s++;
    }
  else if (*s == '-')
    {
      sign = 1;
      s++;
    }

  if ((int)strlen (s) != 4)
    return -1;

  for (int i = 0; i < 4; i++)
    {
      char ch = s[i];
      int digit;

      if (ch >= '0' && ch <= '9')
        digit = ch - '0';
      else if (ch >= 'A' && ch <= 'F')
        digit = 10 + (ch - 'A');
      else if (ch >= 'a' && ch <= 'f')
        digit = 10 + (ch - 'a');
      else
        return -1;

      mag = (mag << 4) | digit;
    }

  mag &= 0x3FFF;
  *value = (sign << 14) | mag;
  return 0;
}

static int
command_from_name (const char *name, int *command)
{
  if (name == NULL || command == NULL)
    return -1;

  if (strcmp (name, "READ") == 0)
    *command = 0x10;
  else if (strcmp (name, "WRITE") == 0)
    *command = 0x11;
  else if (strcmp (name, "LOAD") == 0)
    *command = 0x20;
  else if (strcmp (name, "STORE") == 0)
    *command = 0x21;
  else if (strcmp (name, "ADD") == 0)
    *command = 0x30;
  else if (strcmp (name, "SUB") == 0)
    *command = 0x31;
  else if (strcmp (name, "DIVIDE") == 0)
    *command = 0x32;
  else if (strcmp (name, "MUL") == 0)
    *command = 0x33;
  else if (strcmp (name, "JUMP") == 0)
    *command = 0x40;
  else if (strcmp (name, "JNEG") == 0)
    *command = 0x41;
  else if (strcmp (name, "JZ") == 0)
    *command = 0x42;
  else if (strcmp (name, "HALT") == 0)
    *command = 0x43;
  else
    return -1;

  return 0;
}

static int
assemble_line (char *line, int lineno)
{
  char *comment;
  char *p;
  char *addr_tok;
  char *cmd_tok;
  char *arg_tok;
  int addr;
  int command;
  int operand;
  int value;

  comment = strchr (line, ';');
  if (comment != NULL)
    *comment = '\0';

  trim_right (line);
  p = line;
  trim_left (&p);

  if (*p == '\0')
    return 0;

  addr_tok = strtok (p, " \t");
  cmd_tok = strtok (NULL, " \t");
  arg_tok = strtok (NULL, " \t");

  if (addr_tok == NULL || cmd_tok == NULL || arg_tok == NULL)
    {
      fprintf (stderr, "Ошибка в строке %d: недостаточно полей\n", lineno);
      return -1;
    }

  if (parse_address (addr_tok, &addr) != 0)
    {
      fprintf (stderr, "Ошибка в строке %d: неверный адрес\n", lineno);
      return -1;
    }

  if (strcmp (cmd_tok, "=") == 0)
    {
      if (parse_data_value (arg_tok, &value) != 0)
        {
          fprintf (stderr, "Ошибка в строке %d: неверное значение данных\n",
                   lineno);
          return -1;
        }

      if (sc_memorySet (addr, value) != 0)
        {
          fprintf (stderr, "Ошибка в строке %d: не удалось записать данные\n",
                   lineno);
          return -1;
        }

      return 0;
    }

  if (command_from_name (cmd_tok, &command) != 0)
    {
      fprintf (stderr, "Ошибка в строке %d: неизвестная команда '%s'\n", lineno,
               cmd_tok);
      return -1;
    }

  if (parse_operand (arg_tok, &operand) != 0)
    {
      fprintf (stderr, "Ошибка в строке %d: неверный операнд\n", lineno);
      return -1;
    }

  if (sc_commandEncode (0, command, operand, &value) != 0)
    {
      fprintf (stderr, "Ошибка в строке %d: не удалось закодировать команду\n",
               lineno);
      return -1;
    }

  if (sc_memorySet (addr, value) != 0)
    {
      fprintf (stderr, "Ошибка в строке %d: не удалось записать команду\n",
               lineno);
      return -1;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  FILE *fp;
  char line[MAX_LINE_LEN];
  int lineno = 0;

  if (argc != 3)
    {
      fprintf (stderr, "Использование: sat файл.sa файл.o\n");
      return 1;
    }

  fp = fopen (argv[1], "r");
  if (fp == NULL)
    {
      perror ("Не удалось открыть входной файл");
      return 1;
    }

  if (sc_memoryInit () != 0)
    {
      fprintf (stderr, "Не удалось инициализировать память\n");
      fclose (fp);
      return 1;
    }

  while (fgets (line, sizeof (line), fp) != NULL)
    {
      lineno++;
      if (assemble_line (line, lineno) != 0)
        {
          fclose (fp);
          return 1;
        }
    }

  fclose (fp);

  if (sc_memorySave (argv[2]) != 0)
    {
      fprintf (stderr, "Не удалось сохранить выходной файл\n");
      return 1;
    }

  return 0;
}
