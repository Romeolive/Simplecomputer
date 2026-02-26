#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "myBigChars.h"
#include "mySimpleComputer.h"
#include "myTerm.h"

/* Требования по размеру */
#define MIN_ROWS 27
#define MIN_COLS 100

/* Координаты по рисунку */
#define MEM_TOP 1
#define MEM_LEFT 1
#define MEM_H 12
#define MEM_W 62

#define ACC_TOP 1
#define ACC_LEFT 64
#define ACC_H 3
#define ACC_W 23

#define FLAGS_TOP 1
#define FLAGS_LEFT 88
#define FLAGS_H 3
#define FLAGS_W 23

#define IC_TOP 4
#define IC_LEFT 64
#define IC_H 3
#define IC_W 23

#define CMD_TOP 4
#define CMD_LEFT 88
#define CMD_H 3
#define CMD_W 23

#define BIG_TOP 7
#define BIG_LEFT 64
#define BIG_H 8
#define BIG_W 47

/* формат ниже bigcell */
#define FMT_TOP 15
#define FMT_LEFT 1
#define FMT_H 3
#define FMT_W 62

#define CACHE_TOP 18
#define CACHE_LEFT 1
#define CACHE_H 7
#define CACHE_W 62

#define INOUT_TOP 18
#define INOUT_LEFT 64
#define INOUT_H 7
#define INOUT_W 11

#define KEYS_TOP 18
#define KEYS_LEFT 76
#define KEYS_H 7
#define KEYS_W 35

#define INPUT_TOP 26
#define INPUT_LEFT 1

static void
title_center (int top, int left, int width, const char *title,
              enum colors color)
{
  int len = (int)strlen (title);
  int pos = left + (width - len) / 2;
  if (pos < left + 1)
    pos = left + 1;

  mt_setfgcolor (color);
  mt_gotoXY (top, pos);
  fputs (title, stdout);
  mt_setdefaultcolor ();
}

static void
print_in_box (int row, int col, int inner_w, const char *s)
{
  /* inner_w = ширина внутри рамки (без двух вертикальных линий) */
  mt_gotoXY (row, col);

  /* очистить строку внутри рамки */
  for (int i = 0; i < inner_w; i++)
    putchar (' ');
  mt_gotoXY (row, col);

  /* печать с обрезкой */
  for (int i = 0; s[i] != '\0' && i < inner_w; i++)
    putchar (s[i]);
}

static void
draw_memory (int selected)
{
  bc_box (MEM_TOP, MEM_LEFT, MEM_H, MEM_W);
  title_center (MEM_TOP, MEM_LEFT, MEM_W, "Оперативная память", COLOR_RED);

  int start_row = MEM_TOP + 1;
  int start_col = MEM_LEFT + 2;

  for (int i = 0; i < 100; i++)
    {
      int r = start_row + (i / 10);
      int c = start_col + (i % 10) * 6;

      mt_gotoXY (r, c);

      if (i == selected)
        printCell (i, COLOR_BLACK, COLOR_CYAN);
      else
        printCell (i, COLOR_WHITE, COLOR_BLACK);
    }
}

static void
draw_accumulator (void)
{
  bc_box (ACC_TOP, ACC_LEFT, ACC_H, ACC_W);
  title_center (ACC_TOP, ACC_LEFT, ACC_W, "Аккумулятор", COLOR_RED);

  int acc = 0;
  sc_accumulatorGet (&acc);

  mt_gotoXY (ACC_TOP + 1, ACC_LEFT + 2);
  printf ("sc: %+05d  hex: %04X", acc, acc & 0xFFFF);
}

static void
draw_flags (void)
{
  bc_box (FLAGS_TOP, FLAGS_LEFT, FLAGS_H, FLAGS_W);
  title_center (FLAGS_TOP, FLAGS_LEFT, FLAGS_W, "Регистр флагов", COLOR_RED);

  mt_gotoXY (FLAGS_TOP + 1, FLAGS_LEFT + 2);
  printFlags ();
}

static void
draw_icounter (void)
{
  bc_box (IC_TOP, IC_LEFT, IC_H, IC_W);
  title_center (IC_TOP, IC_LEFT, IC_W, "Счётчик команд", COLOR_RED);

  int ic = 0;
  sc_icounterGet (&ic);

  mt_gotoXY (IC_TOP + 1, IC_LEFT + 2);
  printf ("T: %02d   IC: %04X", 0, ic & 0xFFFF);
}

static void
draw_command (int addr)
{
  bc_box (CMD_TOP, CMD_LEFT, CMD_H, CMD_W);
  title_center (CMD_TOP, CMD_LEFT, CMD_W, "Команда", COLOR_RED);

  int value = 0;
  sc_memoryGet (addr, &value);

  int sign = 0, cmd = 0, op = 0;
  sc_commandDecode (value, &sign, &cmd, &op);

  mt_gotoXY (CMD_TOP + 1, CMD_LEFT + 2);
  printf ("%c %02X : %02X", sign ? '-' : '+', cmd & 0xFF, op & 0xFF);
}

static void
draw_bigcell (int addr)
{
  bc_box (BIG_TOP, BIG_LEFT, BIG_H, BIG_W);
  title_center (BIG_TOP, BIG_LEFT, BIG_W, "Редактируемая ячейка (увеличенно)",
                COLOR_RED);

  int value = 0;
  sc_memoryGet (addr, &value);

  /* "+FFFF" или "-FFFF" */
  char buf[6];
  buf[0] = (((value >> 14) & 1) ? '-' : '+');
  snprintf (buf + 1, sizeof (buf) - 1, "%04X", value & 0x3FFF);

  const int big_row = BIG_TOP + 2;
  const int big_col = BIG_LEFT + 3;
  const int glyph_w = 8;
  const int glyph_gap = 1;
  const int step = glyph_w + glyph_gap;

  for (int i = 0; i < 5; i++)
    {
      bc_printbigchar (buf[i], big_row, big_col + i * step, COLOR_WHITE,
                       COLOR_BLACK);
    }

  /* подпись внутри рамки */
  mt_setfgcolor (COLOR_BLUE);
  mt_gotoXY (big_row + 8, BIG_LEFT + 2);
  printf ("Номер редактируемой ячейки: %03d", addr);
  mt_setdefaultcolor ();
}

static void
draw_format (int addr)
{
  bc_box (FMT_TOP, FMT_LEFT, FMT_H, FMT_W);
  title_center (FMT_TOP, FMT_LEFT, FMT_W, "Редактируемая ячейка (формат)",
                COLOR_RED);

  int value = 0;
  sc_memoryGet (addr, &value);

  mt_gotoXY (FMT_TOP + 1, FMT_LEFT + 2);
  printf ("dec: %05d | oct: %05o | hex: %04X | bin: ", value, value,
          value & 0xFFFF);

  for (int i = 14; i >= 0; i--)
    putchar (((value >> i) & 1) ? '1' : '0');
}

static void
draw_cache (void)
{
  bc_box (CACHE_TOP, CACHE_LEFT, CACHE_H, CACHE_W);
  title_center (CACHE_TOP, CACHE_LEFT, CACHE_W, "Кеш процессора", COLOR_GREEN);

  int base_rows[5] = { 80, 10, 30, 50, 60 };

  for (int r = 0; r < 5; r++)
    {
      mt_gotoXY (CACHE_TOP + 1 + r, CACHE_LEFT + 2);
      printf ("%02d: ", base_rows[r]);

      for (int i = 0; i < 10; i++)
        {
          int addr = (base_rows[r] + i) % 100;
          printCell (addr, COLOR_WHITE, COLOR_BLACK);
        }
    }
}

static void
draw_inout (int selected)
{
  bc_box (INOUT_TOP, INOUT_LEFT, INOUT_H, INOUT_W);
  title_center (INOUT_TOP, INOUT_LEFT, INOUT_W, "IN--OUT", COLOR_GREEN);

  for (int i = 0; i < 5; i++)
    {
      mt_gotoXY (INOUT_TOP + 1 + i, INOUT_LEFT + 1);
      printTerm (i, i == selected);
    }
}

static void
draw_keys (void)
{
  bc_box (KEYS_TOP, KEYS_LEFT, KEYS_H, KEYS_W);
  title_center (KEYS_TOP, KEYS_LEFT, KEYS_W, "Клавиши", COLOR_GREEN);

  const int inner_w = KEYS_W - 2;
  const int text_col = KEYS_LEFT + 1; /* внутри рамки сразу после '|' */

  print_in_box (KEYS_TOP + 1, text_col, inner_w,
                "a/d - move   n - write demo   r - randomize");
  print_in_box (KEYS_TOP + 2, text_col, inner_w,
                "l - load     s - save         q - quit");
}

static void
draw_input_prompt (void)
{
  mt_gotoXY (INPUT_TOP, INPUT_LEFT);
  mt_setfgcolor (COLOR_WHITE);
  fputs ("Команда: a/d/n/r/l/s/q", stdout);
  mt_setdefaultcolor ();
}

static void
read_line (char *buf, size_t bufsz)
{
  if (buf == NULL || bufsz == 0)
    return;

  if (fgets (buf, (int)bufsz, stdin) == NULL)
    {
      buf[0] = '\0';
      return;
    }

  buf[strcspn (buf, "\n")] = '\0';
}

static void
consume_rest_of_line (void)
{
  int c;
  while ((c = getchar ()) != '\n' && c != EOF)
    {
    }
}

int
main (void)
{
  int rows, cols;

  if (!isatty (STDOUT_FILENO))
    {
      fprintf (stderr, "Ошибка: stdout не является терминалом.\n");
      return 1;
    }

  if (mt_getscreensize (&rows, &cols) != 0)
    {
      fprintf (stderr, "Ошибка: не удалось определить размер терминала.\n");
      return 1;
    }

  if (rows < MIN_ROWS || cols < MIN_COLS)
    {
      fprintf (stderr,
               "Ошибка: маленький терминал (%dx%d). Нужно минимум %dx%d.\n",
               rows, cols, MIN_ROWS, MIN_COLS);
      return 1;
    }

  setvbuf (stdout, NULL, _IONBF, 0);

  sc_memoryInit ();
  sc_regInit ();
  sc_accumulatorInit ();
  sc_icounterInit ();

  /* демо */
  sc_memorySet (0, 0x3434);
  sc_memorySet (1, 0x1000);
  sc_memorySet (2, 0x1102);
  sc_memorySet (3, 0x1103);
  sc_accumulatorSet (0);
  sc_icounterSet (0x000E);

  int selected = 0;

  mt_setcursorvisible (0);

  for (;;)
    {
      mt_clrscr ();

      draw_memory (selected);
      draw_accumulator ();
      draw_flags ();
      draw_icounter ();
      draw_command (selected);
      draw_bigcell (selected);

      draw_format (selected);
      draw_cache ();
      draw_inout (selected);
      draw_keys ();
      draw_input_prompt ();

      mt_gotoXY (rows, 1);
      mt_setdefaultcolor ();
      printf ("Введите команду: ");
      fflush (stdout);

      int ch = getchar ();
      if (ch == EOF)
        break;

      if (ch == '\n')
        continue;

      consume_rest_of_line ();

      if (ch == 'q')
        break;

      if (ch == 'd')
        {
          selected = (selected + 1) % 100;
        }
      else if (ch == 'a')
        {
          selected = (selected + 99) % 100;
        }
      else if (ch == 'n')
        {
          int newv = (selected * 37 + 0x123) & 0x3FFF; /* 14 бит */
          sc_memorySet (selected, newv);
        }
      else if (ch == 'r')
        {
          for (int i = 0; i < 20; i++)
            {
              int addr = (i * 7 + 13) % 100;
              int val = (addr * 91 + 0x2A) & 0x3FFF;
              sc_memorySet (addr, val);
            }
        }
      else if (ch == 's')
        {
          char filename[128];

          mt_gotoXY (INPUT_TOP, INPUT_LEFT);
          mt_setfgcolor (COLOR_WHITE);
          fputs ("Введите имя файла для сохранения: ", stdout);
          mt_setdefaultcolor ();
          fflush (stdout);

          read_line (filename, sizeof (filename));
          if (filename[0] != '\0')
            sc_memorySave (filename);
        }
      else if (ch == 'l')
        {
          char filename[128];

          mt_gotoXY (INPUT_TOP, INPUT_LEFT);
          mt_setfgcolor (COLOR_WHITE);
          fputs ("Введите имя файла для загрузки: ", stdout);
          mt_setdefaultcolor ();
          fflush (stdout);

          read_line (filename, sizeof (filename));
          if (filename[0] != '\0')
            sc_memoryLoad (filename);
        }
    }

  mt_setdefaultcolor ();
  mt_setcursorvisible (1);
  mt_gotoXY (rows, 1);
  putchar ('\n');

  return 0;
}
