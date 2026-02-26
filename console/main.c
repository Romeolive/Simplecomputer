#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "mySimpleComputer.h"
#include "myTerm.h"

#define MIN_ROWS 24
#define MIN_COLS 100

/* Геометрия */
#define MEM_TOP 1
#define MEM_LEFT 1
#define MEM_W 66
#define MEM_H 12

#define RIGHT_TOP 1
#define RIGHT_LEFT 70
#define RIGHT_W 30

#define BIG_TOP 14
#define BIG_LEFT 1
#define BIG_W 66
#define BIG_H 7

#define FMT_TOP 14
#define FMT_LEFT 70
#define FMT_W 30
#define FMT_H 7

#define KEYS_TOP 21
#define KEYS_LEFT 1
#define KEYS_W 66
#define KEYS_H 4

#define INOUT_TOP 21
#define INOUT_LEFT 70
#define INOUT_W 30
#define INOUT_H 4

/* Псевдографика (Unicode) */
#define TL "┌"
#define TR "┐"
#define BL "└"
#define BR "┘"
#define HL "─"
#define VL "│"

static void repeat(const char *s, int n)
{
  for (int i = 0; i < n; i++)
    fputs(s, stdout);
}

/*
 * Рамка: рисуем линию полностью, затем печатаем title поверх неё
 * (без strlen/центровки, чтобы UTF-8/русский не ломали геометрию).
 */
static void box(int t, int l, int h, int w, const char *title)
{
  /* верх */
  mt_gotoXY(t, l);
  fputs(TL, stdout);
  repeat(HL, w - 2);
  fputs(TR, stdout);

  /* бока */
  for (int r = 1; r < h - 1; r++)
    {
      mt_gotoXY(t + r, l);
      fputs(VL, stdout);
      mt_gotoXY(t + r, l + w - 1);
      fputs(VL, stdout);
    }

  /* низ */
  mt_gotoXY(t + h - 1, l);
  fputs(BL, stdout);
  repeat(HL, w - 2);
  fputs(BR, stdout);

  /* заголовок */
  if (title && *title)
    {
      mt_gotoXY(t, l + 2);
      mt_setfgcolor(COLOR_RED);
      fputs(title, stdout);
      mt_setdefaultcolor();
    }
}

static void draw_memory(int selected)
{
  box(MEM_TOP, MEM_LEFT, MEM_H, MEM_W, "Оперативная память");

  int row = MEM_TOP + 1;
  int col = MEM_LEFT + 2;

  /* подписи строк 00: 10: ... 90: */
  for (int r = 0; r < 10; r++)
    {
      mt_gotoXY(row + r, col);
      printf("%02X:", r * 10);
    }

  /* 10x10 ячеек */
  for (int i = 0; i < 100; i++)
    {
      int r = row + (i / 10);
      int c = col + 4 + (i % 10) * 6; /* +4 после "XX:" */

      mt_gotoXY(r, c);

      if (i == selected)
        printCell(i, COLOR_BLACK, COLOR_CYAN);
      else
        printCell(i, COLOR_WHITE, COLOR_BLACK);
    }
}

static void draw_registers(int top)
{
  box(top, RIGHT_LEFT, 3, RIGHT_W, "Аккумулятор");
  box(top + 3, RIGHT_LEFT, 3, RIGHT_W, "Счётчик команд");
  box(top + 6, RIGHT_LEFT, 3, RIGHT_W, "Регистр флагов");
  box(top + 9, RIGHT_LEFT, 3, RIGHT_W, "Команда");

  mt_gotoXY(top + 1, RIGHT_LEFT + 2);
  fputs("sc: ", stdout);
  printAccumulator();

  mt_gotoXY(top + 4, RIGHT_LEFT + 2);
  fputs("ic: ", stdout);
  printCounters();

  mt_gotoXY(top + 7, RIGHT_LEFT + 2);
  fputs("flags: ", stdout);
  printFlags();

  mt_gotoXY(top + 10, RIGHT_LEFT + 2);
  fputs("cmd: ", stdout);
  printOperation();
}

static void draw_big(int addr)
{
  box(BIG_TOP, BIG_LEFT, BIG_H, BIG_W, "Редактируемая ячейка (увеличенно)");

  mt_gotoXY(BIG_TOP + 3, BIG_LEFT + 20);
  printCell(addr, COLOR_WHITE, COLOR_BLACK);

  mt_gotoXY(BIG_TOP + 5, BIG_LEFT + 2);
  printf("Номер редактируемой ячейки: %02d", addr);
}

static void draw_format(int addr)
{
  int value;
  box(FMT_TOP, FMT_LEFT, FMT_H, FMT_W, "Ред. ячейка (формат)");

  if (sc_memoryGet(addr, &value) != 0)
    return;

  mt_gotoXY(FMT_TOP + 1, FMT_LEFT + 2);
  printf("DEC: %6d", value);

  mt_gotoXY(FMT_TOP + 2, FMT_LEFT + 2);
  printf("OCT: %6o", value);

  mt_gotoXY(FMT_TOP + 3, FMT_LEFT + 2);
  printf("HEX: %6X", value);

  mt_gotoXY(FMT_TOP + 4, FMT_LEFT + 2);
  fputs("BIN: ", stdout);

  for (int i = 14; i >= 0; i--)
    {
      putchar(((value >> i) & 1) ? '1' : '0');
      if (i % 4 == 0 && i != 0)
        putchar(' ');
    }
}

static void draw_keys(void)
{
  box(KEYS_TOP, KEYS_LEFT, KEYS_H, KEYS_W, "Клавиши");

  mt_gotoXY(KEYS_TOP + 1, KEYS_LEFT + 2);
  fputs("l-load s-save r-run t-step i-reset q-quit", stdout);
}

static void draw_inout(int sel)
{
  box(INOUT_TOP, INOUT_LEFT, INOUT_H, INOUT_W, "IN-OUT");

  for (int i = 0; i < 3; i++)
    {
      mt_gotoXY(INOUT_TOP + 1 + i, INOUT_LEFT + 2);
      printTerm(i, i == sel);
    }
}

int main(void)
{
  setlocale(LC_ALL, "");

  int rows, cols, ic;

  if (mt_getscreensize(&rows, &cols) != 0 || rows < MIN_ROWS || cols < MIN_COLS)
    {
      fprintf(stderr, "Терминал слишком мал. Минимум %dx%d\n", MIN_ROWS, MIN_COLS);
      return 1;
    }

  sc_memoryInit();
  sc_regInit();
  sc_accumulatorInit();
  sc_icounterInit();

  sc_icounterSet(0);
  sc_memorySet(0, 0x051F);
  sc_memorySet(1, 0x451F);
  sc_accumulatorSet(0x1234);
  sc_regSet(SC_FLAG_OVERFLOW, 1);

  sc_icounterGet(&ic);

  mt_clrscr();
  mt_setcursorvisible(0);

  draw_memory(ic);
  draw_registers(RIGHT_TOP);
  draw_big(ic);
  draw_format(ic);
  draw_keys();
  draw_inout(0);

  mt_setcursorvisible(1);
  mt_gotoXY(rows, 1);

  return 0;
}
