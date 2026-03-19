#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#include "myBigChars.h"
#include "myReadKey.h"
#include "mySimpleComputer.h"
#include "myTerm.h"

/* Требования по размеру */
#define MIN_ROWS 30
#define MIN_COLS 100

/* Память: 128 ячеек = 12 полных строк (по 10) + 1 строка (8) */
#define MEM_CELLS_TOTAL SC_MEMORY_SIZE
#define MEM_COLS_PER_ROW 10
#define MEM_FULL_ROWS 12
#define MEM_LAST_ROW_CELLS 8
#define MEM_ROWS_TOTAL 13

/* Координаты по рисунку */
#define MEM_TOP 1
#define MEM_LEFT 1
#define MEM_H (MEM_ROWS_TOTAL + 2)
#define MEM_W 63

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
#define BIG_H 12
#define BIG_W 47

#define FMT_TOP (MEM_TOP + MEM_H)
#define FMT_LEFT 1
#define FMT_H 3
#define FMT_W 62

#define CACHE_TOP (FMT_TOP + FMT_H)
#define CACHE_LEFT 1
#define CACHE_H 7
#define CACHE_W 62

#define INOUT_TOP (FMT_TOP + FMT_H)
#define INOUT_LEFT 64
#define INOUT_H 7
#define INOUT_W 14

#define KEYS_TOP (FMT_TOP + FMT_H)
#define KEYS_LEFT 79
#define KEYS_H 7
#define KEYS_W 32

#define INPUT_TOP (CACHE_TOP + CACHE_H + 1)
#define INPUT_LEFT 1

#define CMD_READ 0x10
#define CMD_WRITE 0x11
#define CMD_LOAD 0x20
#define CMD_STORE 0x21
#define CMD_ADD 0x30
#define CMD_SUB 0x31
#define CMD_DIVIDE 0x32
#define CMD_MUL 0x33
#define CMD_JUMP 0x40
#define CMD_JNEG 0x41
#define CMD_JZ 0x42
#define CMD_HALT 0x43

static volatile sig_atomic_t g_run = 0;
static volatile sig_atomic_t g_alarm = 0;
static volatile sig_atomic_t g_reset = 0;
static volatile sig_atomic_t g_need_redraw = 1;

static struct itimerval g_timer
    = { .it_interval = { 0, 500000 }, .it_value = { 0, 500000 } };

/* Буфер блока IN-OUT */
static char io_lines[5][32];

/* ---------------- util ---------------- */

static int
term_width_utf8 (const char *s)
{
  if (!s)
    return 0;

  mbstate_t st;
  memset (&st, 0, sizeof (st));

  int w = 0;
  const char *p = s;

  while (*p)
    {
      wchar_t wc;
      size_t n = mbrtowc (&wc, p, MB_CUR_MAX, &st);
      if (n == (size_t)-1 || n == (size_t)-2)
        {
          memset (&st, 0, sizeof (st));
          p++;
          w += 1;
          continue;
        }
      if (n == 0)
        break;

      int cw = wcwidth (wc);
      if (cw < 0)
        cw = 1;
      w += cw;
      p += (int)n;
    }

  return w;
}

static void
title_center (int top, int left, int width, const char *title,
              enum colors color)
{
  const int inner_l = left + 1;
  const int inner_r = left + width - 2;

  mt_gotoXY (top, inner_l);
  for (int i = 0; i < (width - 2); i++)
    putchar ('-');

  int tw = term_width_utf8 (title);
  int total = tw + 2;

  if (total > (width - 2))
    return;

  int start = left + (width - total) / 2;
  if (start < inner_l)
    start = inner_l;
  if (start + total - 1 > inner_r)
    start = inner_r - total + 1;

  mt_setfgcolor (color);
  mt_gotoXY (top, start);
  putchar (' ');
  fputs (title, stdout);
  putchar (' ');
  mt_setdefaultcolor ();
}

static void
print_in_box (int row, int col, int inner_w, const char *s)
{
  mt_gotoXY (row, col);
  for (int i = 0; i < inner_w; i++)
    putchar (' ');
  mt_gotoXY (row, col);

  for (int i = 0; s[i] != '\0' && i < inner_w; i++)
    putchar (s[i]);
}

static void
read_line (char *buf, size_t bufsz)
{
  if (!buf || bufsz == 0)
    return;

  if (!fgets (buf, (int)bufsz, stdin))
    {
      buf[0] = '\0';
      return;
    }
  buf[strcspn (buf, "\n")] = '\0';
}

static int
hexval (int c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

static int
parse_sc_value (const char *s, int *out)
{
  if (!s || !out)
    return -1;

  while (*s == ' ' || *s == '\t')
    s++;

  int sign = 0;
  if (*s == '+' || *s == '-')
    {
      sign = (*s == '-');
      s++;
    }

  while (*s == ' ' || *s == '\t')
    s++;

  if (strlen (s) != 4)
    return -1;

  int mag = 0;
  for (int i = 0; i < 4; i++)
    {
      int hv = hexval ((unsigned char)s[i]);
      if (hv < 0)
        return -1;
      mag = (mag << 4) | hv;
    }

  mag &= 0x3FFF;
  *out = (sign ? (1 << 14) : 0) | mag;
  return 0;
}

static int
sc_to_int (int raw)
{
  int sign = (raw >> 14) & 1;
  int mag = raw & 0x3FFF;
  return sign ? -mag : mag;
}

static int
int_to_sc (int value, int *raw)
{
  if (!raw)
    return -1;

  int sign = 0;
  int mag = value;

  if (value < 0)
    {
      sign = 1;
      mag = -value;
    }

  if (mag > 0x3FFF)
    return -1;

  *raw = (sign << 14) | (mag & 0x3FFF);
  return 0;
}

/* ---------------- io block ---------------- */

static void
io_clear (void)
{
  for (int i = 0; i < 5; i++)
    io_lines[i][0] = '\0';
}

static void
io_put_line (int line, const char *text)
{
  if (line < 0 || line >= 5 || text == NULL)
    return;

  snprintf (io_lines[line], sizeof (io_lines[line]), "%s", text);
}

/* ---------------- draw blocks ---------------- */

static void
draw_memory (int selected)
{
  bc_box (MEM_TOP, MEM_LEFT, MEM_H, MEM_W);
  title_center (MEM_TOP, MEM_LEFT, MEM_W, "Оперативная память", COLOR_RED);

  const int start_row = MEM_TOP + 1;
  const int start_col = MEM_LEFT + 2;

  for (int i = 0; i < MEM_CELLS_TOTAL; i++)
    {
      int row = i / MEM_COLS_PER_ROW;
      int col = i % MEM_COLS_PER_ROW;

      if (row == MEM_FULL_ROWS && col >= MEM_LAST_ROW_CELLS)
        continue;

      int r = start_row + row;
      int c = start_col + col * 6;

      mt_gotoXY (r, c);
      if (i == selected)
        printCell (i, COLOR_BLACK, COLOR_CYAN);
      else
        printCell (i, COLOR_WHITE, COLOR_BLACK);
    }

  {
    int tail_row = start_row + MEM_FULL_ROWS;
    int tail_col = start_col + MEM_LAST_ROW_CELLS * 6;
    int inner_right = MEM_LEFT + MEM_W - 2;
    int to_clear = inner_right - tail_col + 1;

    if (to_clear > 0)
      {
        mt_gotoXY (tail_row, tail_col);
        for (int k = 0; k < to_clear; k++)
          putchar (' ');
      }
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

  char buf[6];
  buf[0] = (((value >> 14) & 1) ? '-' : '+');
  snprintf (buf + 1, sizeof (buf) - 1, "%04X", value & 0x3FFF);

  const int big_row = BIG_TOP + 2;
  const int big_col = BIG_LEFT + 2;
  const int glyph_w = 8;
  const int glyph_gap = 1;
  const int step = glyph_w + glyph_gap;

  for (int i = 0; i < 5; i++)
    {
      bc_printbigchar (buf[i], big_row, big_col + i * step, COLOR_WHITE,
                       COLOR_BLACK);
    }

  mt_setfgcolor (COLOR_BLUE);
  mt_gotoXY (BIG_TOP + BIG_H - 2, BIG_LEFT + 2);
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

      for (int i = 0; i < 9; i++)
        {
          int addr = (base_rows[r] + i) % 100;
          printCell (addr, COLOR_WHITE, COLOR_BLACK);
        }
    }
}

static void
draw_inout (int selected)
{
  (void)selected;

  bc_box (INOUT_TOP, INOUT_LEFT, INOUT_H, INOUT_W);
  title_center (INOUT_TOP, INOUT_LEFT, INOUT_W, "IN--OUT", COLOR_GREEN);

  for (int i = 0; i < 5; i++)
    {
      mt_gotoXY (INOUT_TOP + 1 + i, INOUT_LEFT + 1);
      printf ("%-12s", io_lines[i]);
    }
}

static void
draw_keys (void)
{
  bc_box (KEYS_TOP, KEYS_LEFT, KEYS_H, KEYS_W);
  title_center (KEYS_TOP, KEYS_LEFT, KEYS_W, "Клавиши", COLOR_GREEN);

  const int inner_w = KEYS_W - 2;
  const int text_col = KEYS_LEFT + 1;

  print_in_box (KEYS_TOP + 1, text_col, inner_w,
                "Arrows move | Enter edit | Esc quit");
  print_in_box (KEYS_TOP + 2, text_col, inner_w,
                "F5 acc | F6 ic | r run | s step | i reset");
}

static void
draw_input_prompt (void)
{
  mt_gotoXY (INPUT_TOP, INPUT_LEFT);
  mt_setfgcolor (COLOR_WHITE);
  fputs ("Arrows/Enter/F5/F6/r/s/i (ESC - exit)", stdout);
  mt_setdefaultcolor ();
}

/* ---------------- InPlace editing ---------------- */

static void
cell_pos (int addr, int *row, int *col)
{
  const int start_row = MEM_TOP + 1;
  const int start_col = MEM_LEFT + 2;

  int r = addr / MEM_COLS_PER_ROW;
  int c = addr % MEM_COLS_PER_ROW;

  *row = start_row + r;
  *col = start_col + c * 6;
}

static int
is_hex_digit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
         || (c >= 'A' && c <= 'F');
}

static int
hex_to_int (int c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

static int
sc_pack_sign_mag (int sign, int mag)
{
  mag &= 0x3FFF;
  return (sign ? (1 << 14) : 0) | mag;
}

static void
draw_field_5 (int row, int col, const char buf5[6], enum colors fg,
              enum colors bg)
{
  mt_setfgcolor (fg);
  mt_setbgcolor (bg);
  mt_gotoXY (row, col);
  for (int i = 0; i < 5; i++)
    putchar (buf5[i]);
  mt_setdefaultcolor ();
}

static int
inplace_edit_hex4 (int row, int col, int *out_value, int old_value)
{
  char buf[6];
  buf[0] = (((old_value >> 14) & 1) ? '-' : '+');
  snprintf (buf + 1, 5, "%04X", old_value & 0x3FFF);

  int pos = 0;
  mt_setcursorvisible (1);

  for (;;)
    {
      draw_field_5 (row, col, buf, COLOR_BLACK, COLOR_CYAN);
      mt_gotoXY (row, col + pos);

      enum keys k = KEY_UNKNOWN;
      if (rk_readkey (&k) != 0)
        continue;

      if (k == KEY_ESC)
        {
          mt_setcursorvisible (0);
          return -1;
        }

      if (k == KEY_ENTER)
        {
          int sign = (buf[0] == '-') ? 1 : 0;
          int mag = 0;

          for (int i = 1; i <= 4; i++)
            {
              int hv = hex_to_int ((unsigned char)buf[i]);
              if (hv < 0)
                {
                  mag = -1;
                  break;
                }
              mag = (mag << 4) | hv;
            }

          if (mag < 0)
            continue;

          if (out_value)
            *out_value = sc_pack_sign_mag (sign, mag);

          mt_setcursorvisible (0);
          return 0;
        }

      if (k == KEY_LEFT)
        {
          if (pos > 0)
            pos--;
          continue;
        }
      if (k == KEY_RIGHT)
        {
          if (pos < 4)
            pos++;
          continue;
        }

      if (k == KEY_CHAR)
        {
          int ch = rk_last_char;

          if (ch == 127 || ch == 8)
            {
              if (pos > 0)
                pos--;
              if (pos == 0)
                buf[0] = '+';
              else
                buf[pos] = '0';
              continue;
            }

          if (pos == 0 && (ch == '+' || ch == '-'))
            {
              buf[0] = (char)ch;
              pos = 1;
              continue;
            }

          if (pos >= 1 && pos <= 4 && is_hex_digit (ch))
            {
              if (ch >= 'a' && ch <= 'f')
                ch = ch - 'a' + 'A';
              buf[pos] = (char)ch;
              if (pos < 4)
                pos++;
              continue;
            }
        }
    }
}

static int
inplace_edit_dec (int row, int col, int width, int minv, int maxv,
                  int *out_value, int old_value)
{
  char buf[8];
  snprintf (buf, sizeof (buf), "%*d", width, old_value);

  int pos = 0;
  mt_setcursorvisible (1);

  for (;;)
    {
      mt_setfgcolor (COLOR_BLACK);
      mt_setbgcolor (COLOR_CYAN);
      mt_gotoXY (row, col);
      for (int i = 0; i < width; i++)
        putchar (buf[i]);
      mt_setdefaultcolor ();

      mt_gotoXY (row, col + pos);

      enum keys k = KEY_UNKNOWN;
      if (rk_readkey (&k) != 0)
        continue;

      if (k == KEY_ESC)
        {
          mt_setcursorvisible (0);
          return -1;
        }

      if (k == KEY_ENTER)
        {
          int v = 0;
          int seen = 0;
          for (int i = 0; i < width; i++)
            {
              if (buf[i] >= '0' && buf[i] <= '9')
                {
                  v = v * 10 + (buf[i] - '0');
                  seen = 1;
                }
            }

          if (!seen)
            v = old_value;

          if (v < minv || v > maxv)
            continue;

          if (out_value)
            *out_value = v;

          mt_setcursorvisible (0);
          return 0;
        }

      if (k == KEY_LEFT)
        {
          if (pos > 0)
            pos--;
          continue;
        }
      if (k == KEY_RIGHT)
        {
          if (pos < width - 1)
            pos++;
          continue;
        }

      if (k == KEY_CHAR)
        {
          int ch = rk_last_char;

          if (ch == 127 || ch == 8)
            {
              buf[pos] = ' ';
              if (pos > 0)
                pos--;
              continue;
            }

          if (ch >= '0' && ch <= '9')
            {
              buf[pos] = (char)ch;
              if (pos < width - 1)
                pos++;
              continue;
            }
        }
    }
}

static void
acc_field_pos (int *row, int *col)
{
  *row = ACC_TOP + 1;
  *col = ACC_LEFT + 2 + 4;
}

static void
ic_field_pos (int *row, int *col)
{
  *row = IC_TOP + 1;
  *col = IC_LEFT + 2 + 10;
}

/* ---------------- machine / signals ---------------- */

static void
start_timer (void)
{
  setitimer (ITIMER_REAL, &g_timer, NULL);
}

static void
stop_timer (void)
{
  struct itimerval stop = { { 0, 0 }, { 0, 0 } };
  setitimer (ITIMER_REAL, &stop, NULL);
}

static void
reset_machine (void)
{
  stop_timer ();
  g_run = 0;

  sc_memoryInit ();
  sc_regInit ();
  sc_accumulatorInit ();
  sc_icounterInit ();
  io_clear ();
}

static int
ALU (int command, int operand)
{
  int acc_raw, mem_raw;
  int acc, mem, result, result_raw;

  if (sc_accumulatorGet (&acc_raw) != 0)
    return -1;
  if (sc_memoryGet (operand, &mem_raw) != 0)
    {
      sc_regSet (SC_FLAG_OUTMEM, 1);
      return -1;
    }

  acc = sc_to_int (acc_raw);
  mem = sc_to_int (mem_raw);

  switch (command)
    {
    case CMD_ADD:
      result = acc + mem;
      break;
    case CMD_SUB:
      result = acc - mem;
      break;
    case CMD_DIVIDE:
      if (mem == 0)
        {
          sc_regSet (SC_FLAG_DIVZERO, 1);
          return -1;
        }
      result = acc / mem;
      break;
    case CMD_MUL:
      result = acc * mem;
      break;
    default:
      sc_regSet (SC_FLAG_BADCOMMAND, 1);
      return -1;
    }

  if (int_to_sc (result, &result_raw) != 0)
    {
      sc_regSet (SC_FLAG_OVERFLOW, 1);
      return -1;
    }

  return sc_accumulatorSet (result_raw);
}

static void
redraw_now (int selected)
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
  g_need_redraw = 0;
}

static void
CU (void)
{
  int ic;
  int value;
  int sign, command, operand;
  int acc_raw;

  if (sc_icounterGet (&ic) != 0)
    return;

  if (ic < 0 || ic >= SC_MEMORY_SIZE)
    {
      sc_regSet (SC_FLAG_OUTMEM, 1);
      g_run = 0;
      stop_timer ();
      io_clear ();
      io_put_line (0, "BAD ADDRESS");
      return;
    }

  if (sc_memoryGet (ic, &value) != 0)
    {
      sc_regSet (SC_FLAG_OUTMEM, 1);
      g_run = 0;
      stop_timer ();
      io_clear ();
      io_put_line (0, "MEM ERROR");
      return;
    }

  if (sc_commandDecode (value, &sign, &command, &operand) != 0)
    {
      sc_regSet (SC_FLAG_BADCOMMAND, 1);
      g_run = 0;
      stop_timer ();
      io_clear ();
      io_put_line (0, "BAD CMD");
      return;
    }

  switch (command)
    {
    case CMD_READ:
      {
        char buf[64];
        int newv;

        rk_mytermregime (0, 0, 0, 1, 1);

        io_clear ();
        io_put_line (0, "READ");
        snprintf (io_lines[1], sizeof (io_lines[1]), "addr %02d", operand);
        snprintf (io_lines[2], sizeof (io_lines[2]), "> ");
        g_need_redraw = 1;
        redraw_now (ic);

        mt_gotoXY (INOUT_TOP + 3, INOUT_LEFT + 3);
        fflush (stdout);

        read_line (buf, sizeof (buf));

        if (parse_sc_value (buf, &newv) == 0)
          {
            sc_memorySet (operand, newv);
            snprintf (io_lines[3], sizeof (io_lines[3]), "[%02d]=%c%04X",
                      operand, ((newv >> 14) & 1) ? '-' : '+', newv & 0x3FFF);
          }
        else
          {
            io_put_line (3, "INPUT ERROR");
          }

        rk_mytermregime (1, 0, 1, 0, 0);
      }
      break;

    case CMD_WRITE:
      {
        int outv = 0;
        sc_memoryGet (operand, &outv);

        io_clear ();
        io_put_line (0, "WRITE");
        snprintf (io_lines[1], sizeof (io_lines[1]), "[%02d] =", operand);
        snprintf (io_lines[2], sizeof (io_lines[2]), "%c%04X",
                  ((outv >> 14) & 1) ? '-' : '+', outv & 0x3FFF);
      }
      break;

    case CMD_LOAD:
      if (sc_memoryGet (operand, &value) == 0)
        {
          sc_accumulatorSet (value);
          io_clear ();
          io_put_line (0, "LOAD");
          snprintf (io_lines[1], sizeof (io_lines[1]), "ACC<-[%02d]", operand);
        }
      else
        {
          sc_regSet (SC_FLAG_OUTMEM, 1);
          g_run = 0;
          stop_timer ();
          return;
        }
      break;

    case CMD_STORE:
      if (sc_accumulatorGet (&acc_raw) == 0)
        {
          sc_memorySet (operand, acc_raw);
          io_clear ();
          io_put_line (0, "STORE");
          snprintf (io_lines[1], sizeof (io_lines[1]), "[%02d]<-ACC", operand);
        }
      else
        {
          g_run = 0;
          stop_timer ();
          return;
        }
      break;

    case CMD_ADD:
    case CMD_SUB:
    case CMD_DIVIDE:
    case CMD_MUL:
      if (ALU (command, operand) != 0)
        {
          g_run = 0;
          stop_timer ();
          io_clear ();
          io_put_line (0, "ALU ERROR");
          return;
        }
      else
        {
          io_clear ();
          if (command == CMD_ADD)
            io_put_line (0, "ADD");
          else if (command == CMD_SUB)
            io_put_line (0, "SUB");
          else if (command == CMD_DIVIDE)
            io_put_line (0, "DIV");
          else if (command == CMD_MUL)
            io_put_line (0, "MUL");

          snprintf (io_lines[1], sizeof (io_lines[1]), "op=[%02d]", operand);
        }
      break;

    case CMD_JUMP:
      io_clear ();
      io_put_line (0, "JUMP");
      snprintf (io_lines[1], sizeof (io_lines[1]), "to %02d", operand);
      sc_icounterSet (operand);
      g_need_redraw = 1;
      return;

    case CMD_JNEG:
      io_clear ();
      io_put_line (0, "JNEG");
      if (sc_accumulatorGet (&acc_raw) == 0 && ((acc_raw >> 14) & 1))
        {
          snprintf (io_lines[1], sizeof (io_lines[1]), "jump %02d", operand);
          sc_icounterSet (operand);
          g_need_redraw = 1;
          return;
        }
      else
        {
          io_put_line (1, "no jump");
        }
      break;

    case CMD_JZ:
      io_clear ();
      io_put_line (0, "JZ");
      if (sc_accumulatorGet (&acc_raw) == 0 && ((acc_raw & 0x3FFF) == 0))
        {
          snprintf (io_lines[1], sizeof (io_lines[1]), "jump %02d", operand);
          sc_icounterSet (operand);
          g_need_redraw = 1;
          return;
        }
      else
        {
          io_put_line (1, "no jump");
        }
      break;

    case CMD_HALT:
      {
        int outv = 0;

        io_clear ();
        io_put_line (0, "HALT");

        if (sc_memoryGet (21, &outv) == 0)
          {
            io_put_line (1, "RESULT [21]");
            snprintf (io_lines[2], sizeof (io_lines[2]), "%c%04X",
                      ((outv >> 14) & 1) ? '-' : '+', outv & 0x3FFF);
          }

        g_run = 0;
        stop_timer ();
        return;
      }

    default:
      sc_regSet (SC_FLAG_BADCOMMAND, 1);
      g_run = 0;
      stop_timer ();
      io_clear ();
      io_put_line (0, "BAD CMD");
      return;
    }

  sc_icounterSet (ic + 1);
  g_need_redraw = 1;
}

static void
process_tick (int check_ignoreclock, int *selected)
{
  if (check_ignoreclock)
    {
      int ignore = 0;
      sc_regGet (SC_FLAG_IGNORECLOCK, &ignore);
      if (ignore)
        {
          g_need_redraw = 1;
          return;
        }
    }

  CU ();

  int ic = 0;
  if (sc_icounterGet (&ic) == 0 && ic >= 0 && ic < SC_MEMORY_SIZE)
    *selected = ic;

  g_need_redraw = 1;
}

static void
IRC (int signum)
{
  if (signum == SIGALRM)
    {
      g_alarm = 1;
    }
  else if (signum == SIGUSR1)
    {
      g_reset = 1;
    }
}

static int
setup_signals (void)
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = IRC;
  sigemptyset (&sa.sa_mask);

  if (sigaction (SIGALRM, &sa, NULL) != 0)
    return -1;
  if (sigaction (SIGUSR1, &sa, NULL) != 0)
    return -1;

  return 0;
}

/* ---------------- main ---------------- */

int
main (void)
{
  int rows, cols;

  setlocale (LC_ALL, "");

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

  reset_machine ();
  io_put_line (0, "READY");

  int selected = 0;

  mt_setcursorvisible (0);

  rk_mytermsave ();
  rk_mytermregime (1, 0, 1, 0, 0);

  if (setup_signals () != 0)
    {
      rk_mytermrestore ();
      fprintf (stderr,
               "Ошибка: не удалось установить обработчики сигналов.\n");
      return 1;
    }

  for (;;)
    {
      if (g_reset)
        {
          reset_machine ();
          io_put_line (0, "RESET");
          selected = 0;
          g_reset = 0;
          g_need_redraw = 1;
        }

      if (g_alarm)
        {
          g_alarm = 0;

          if (g_run)
            process_tick (1, &selected);
          else
            g_need_redraw = 1;
        }

      if (g_need_redraw)
        redraw_now (selected);

      if (g_run)
        {
          pause ();
          continue;
        }

      enum keys key = KEY_UNKNOWN;
      if (rk_readkey (&key) != 0)
        continue;

      if (key == KEY_ESC)
        break;

      if (key == KEY_ENTER)
        {
          int r, c;
          cell_pos (selected, &r, &c);

          int oldv = 0;
          sc_memoryGet (selected, &oldv);

          int newv;
          if (inplace_edit_hex4 (r, c, &newv, oldv) == 0)
            sc_memorySet (selected, newv);

          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_F5)
        {
          int r, c;
          acc_field_pos (&r, &c);

          int oldv = 0;
          sc_accumulatorGet (&oldv);

          int newv;
          if (inplace_edit_hex4 (r, c, &newv, oldv) == 0)
            sc_accumulatorSet (newv);

          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_F6)
        {
          int r, c;
          ic_field_pos (&r, &c);

          int oldic = 0;
          sc_icounterGet (&oldic);

          int newic;
          if (inplace_edit_dec (r, c, 3, 0, SC_MEMORY_SIZE - 1, &newic, oldic)
              == 0)
            {
              sc_icounterSet (newic);
              selected = newic;
            }

          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_RIGHT)
        {
          selected = (selected + 1) % MEM_CELLS_TOTAL;
          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_LEFT)
        {
          selected = (selected + MEM_CELLS_TOTAL - 1) % MEM_CELLS_TOTAL;
          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_UP)
        {
          if (selected >= MEM_COLS_PER_ROW)
            selected -= MEM_COLS_PER_ROW;
          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_DOWN)
        {
          if (selected + MEM_COLS_PER_ROW < MEM_CELLS_TOTAL)
            selected += MEM_COLS_PER_ROW;
          g_need_redraw = 1;
          continue;
        }

      if (key == KEY_CHAR)
        {
          int ch = rk_last_char;

          if (ch >= 'A' && ch <= 'Z')
            ch = ch - 'A' + 'a';

          if (ch == 'q')
            break;

          if (ch == 'i')
            {
              raise (SIGUSR1);
              continue;
            }

          if (ch == 'r')
            {
              g_run = 1;
              io_clear ();
              io_put_line (0, "RUN");
              start_timer ();
              g_need_redraw = 1;
              continue;
            }

          if (ch == 's')
            {
              /* Один тактовый импульс БЕЗ проверки флага T */
              process_tick (0, &selected);
              continue;
            }

          if (ch == 'w')
            {
              char filename[128];

              rk_mytermregime (0, 0, 0, 1, 1);

              mt_gotoXY (INPUT_TOP, INPUT_LEFT);
              mt_setfgcolor (COLOR_WHITE);
              fputs ("Введите имя файла для сохранения: ", stdout);
              mt_setdefaultcolor ();
              fflush (stdout);

              read_line (filename, sizeof (filename));
              if (filename[0] != '\0')
                {
                  if (sc_memorySave (filename) == 0)
                    {
                      io_clear ();
                      io_put_line (0, "SAVE FILE");
                    }
                  else
                    {
                      io_clear ();
                      io_put_line (0, "SAVE ERROR");
                    }
                }

              rk_mytermregime (1, 0, 1, 0, 0);
              g_need_redraw = 1;
              continue;
            }

          if (ch == 'l')
            {
              char filename[128];

              rk_mytermregime (0, 0, 0, 1, 1);

              mt_gotoXY (INPUT_TOP, INPUT_LEFT);
              mt_setfgcolor (COLOR_WHITE);
              fputs ("Введите имя файла для загрузки: ", stdout);
              mt_setdefaultcolor ();
              fflush (stdout);

              read_line (filename, sizeof (filename));
              if (filename[0] != '\0')
                sc_memoryLoad (filename);

              rk_mytermregime (1, 0, 1, 0, 0);
              io_clear ();
              io_put_line (0, "LOAD FILE");
              g_need_redraw = 1;
              continue;
            }
        }
    }

  stop_timer ();
  rk_mytermrestore ();

  mt_setdefaultcolor ();
  mt_setcursorvisible (1);
  mt_gotoXY (rows, 1);
  putchar ('\n');

  return 0;
}
