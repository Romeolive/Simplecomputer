#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define MEM_H (MEM_ROWS_TOTAL + 2) /* верх + низ рамки + строки памяти */
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

/* Формат под памятью */
#define FMT_TOP (MEM_TOP + MEM_H)
#define FMT_LEFT 1
#define FMT_H 3
#define FMT_W 62

/* Нижние блоки */
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

static int term_width_utf8(const char *s) {
  if (!s)
    return 0;

  mbstate_t st;
  memset(&st, 0, sizeof(st));

  int w = 0;
  const char *p = s;

  while (*p) {
    wchar_t wc;
    size_t n = mbrtowc(&wc, p, MB_CUR_MAX, &st);
    if (n == (size_t)-1 || n == (size_t)-2) {
      memset(&st, 0, sizeof(st));
      p++;
      w += 1;
      continue;
    }
    if (n == 0)
      break;

    int cw = wcwidth(wc);
    if (cw < 0)
      cw = 1;
    w += cw;
    p += (int)n;
  }

  return w;
}

/* Пишем заголовок поверх верхней линии рамки */
static void title_center(int top, int left, int width, const char *title,
                         enum colors color) {
  const int inner_l = left + 1;
  const int inner_r = left + width - 2;

  /* верхняя линия полностью (простые ASCII, чтобы не ломалось в терминалах) */
  mt_gotoXY(top, inner_l);
  for (int i = 0; i < (width - 2); i++)
    putchar('-');

  int tw = term_width_utf8(title);
  int total = tw + 2; /* пробелы вокруг */

  if (total > (width - 2))
    return;

  int start = left + (width - total) / 2;
  if (start < inner_l)
    start = inner_l;
  if (start + total - 1 > inner_r)
    start = inner_r - total + 1;

  mt_setfgcolor(color);
  mt_gotoXY(top, start);
  putchar(' ');
  fputs(title, stdout);
  putchar(' ');
  mt_setdefaultcolor();
}

static void print_in_box(int row, int col, int inner_w, const char *s) {
  mt_gotoXY(row, col);
  for (int i = 0; i < inner_w; i++)
    putchar(' ');
  mt_gotoXY(row, col);

  for (int i = 0; s[i] != '\0' && i < inner_w; i++)
    putchar(s[i]);
}

static void draw_memory(int selected) {
  bc_box(MEM_TOP, MEM_LEFT, MEM_H, MEM_W);
  title_center(MEM_TOP, MEM_LEFT, MEM_W, "Оперативная память", COLOR_RED);

  const int start_row = MEM_TOP + 1;
  const int start_col = MEM_LEFT + 2;

  for (int i = 0; i < MEM_CELLS_TOTAL; i++) {
    /* 0..119 -> 12 строк по 10
       120..127 -> 13-я строка 8 значений */
    int row = i / MEM_COLS_PER_ROW; /* 0..12 */
    int col = i % MEM_COLS_PER_ROW; /* 0..9 */

    /* Для последней строки печатаем только 8 ячеек */
    if (row == MEM_FULL_ROWS && col >= MEM_LAST_ROW_CELLS)
      continue;

    int r = start_row + row;
    int c = start_col + col * 6;

    mt_gotoXY(r, c);
    if (i == selected)
      printCell(i, COLOR_BLACK, COLOR_CYAN);
    else
      printCell(i, COLOR_WHITE, COLOR_BLACK);
  }

  /* Для аккуратности: подчистить "хвост" в последней строке после 8-й ячейки */
  {
    int tail_row = start_row + MEM_FULL_ROWS;
    int tail_col = start_col + MEM_LAST_ROW_CELLS * 6;
    int inner_right = MEM_LEFT + MEM_W - 2;
    int to_clear = inner_right - tail_col + 1;

    if (to_clear > 0) {
      mt_gotoXY(tail_row, tail_col);
      for (int k = 0; k < to_clear; k++)
        putchar(' ');
    }
  }
}

static void draw_accumulator(void) {
  bc_box(ACC_TOP, ACC_LEFT, ACC_H, ACC_W);
  title_center(ACC_TOP, ACC_LEFT, ACC_W, "Аккумулятор", COLOR_RED);

  int acc = 0;
  sc_accumulatorGet(&acc);

  mt_gotoXY(ACC_TOP + 1, ACC_LEFT + 2);
  printf("sc: %+05d  hex: %04X", acc, acc & 0xFFFF);
}

static void draw_flags(void) {
  bc_box(FLAGS_TOP, FLAGS_LEFT, FLAGS_H, FLAGS_W);
  title_center(FLAGS_TOP, FLAGS_LEFT, FLAGS_W, "Регистр флагов", COLOR_RED);

  mt_gotoXY(FLAGS_TOP + 1, FLAGS_LEFT + 2);
  printFlags();
}

static void draw_icounter(void) {
  bc_box(IC_TOP, IC_LEFT, IC_H, IC_W);
  title_center(IC_TOP, IC_LEFT, IC_W, "Счётчик команд", COLOR_RED);

  int ic = 0;
  sc_icounterGet(&ic);

  mt_gotoXY(IC_TOP + 1, IC_LEFT + 2);
  printf("T: %02d   IC: %04X", 0, ic & 0xFFFF);
}

static void draw_command(int addr) {
  bc_box(CMD_TOP, CMD_LEFT, CMD_H, CMD_W);
  title_center(CMD_TOP, CMD_LEFT, CMD_W, "Команда", COLOR_RED);

  int value = 0;
  sc_memoryGet(addr, &value);

  int sign = 0, cmd = 0, op = 0;
  sc_commandDecode(value, &sign, &cmd, &op);

  mt_gotoXY(CMD_TOP + 1, CMD_LEFT + 2);
  printf("%c %02X : %02X", sign ? '-' : '+', cmd & 0xFF, op & 0xFF);
}

static void draw_bigcell(int addr) {
  bc_box(BIG_TOP, BIG_LEFT, BIG_H, BIG_W);
  title_center(BIG_TOP, BIG_LEFT, BIG_W, "Редактируемая ячейка (увеличенно)",
               COLOR_RED);

  int value = 0;
  sc_memoryGet(addr, &value);

  char buf[6];
  buf[0] = (((value >> 14) & 1) ? '-' : '+');
  snprintf(buf + 1, sizeof(buf) - 1, "%04X", value & 0x3FFF);

  const int big_row = BIG_TOP + 2;
  const int big_col = BIG_LEFT + 2;
  const int glyph_w = 8;
  const int glyph_gap = 1;
  const int step = glyph_w + glyph_gap;

  for (int i = 0; i < 5; i++) {
    bc_printbigchar(buf[i], big_row, big_col + i * step, COLOR_WHITE,
                    COLOR_BLACK);
  }

  /* подпись внутри рамки */
  mt_setfgcolor(COLOR_BLUE);
  mt_gotoXY(BIG_TOP + BIG_H - 2, BIG_LEFT + 2);
  printf("Номер редактируемой ячейки: %03d", addr);
  mt_setdefaultcolor();
}

static void draw_format(int addr) {
  bc_box(FMT_TOP, FMT_LEFT, FMT_H, FMT_W);
  title_center(FMT_TOP, FMT_LEFT, FMT_W, "Редактируемая ячейка (формат)",
               COLOR_RED);

  int value = 0;
  sc_memoryGet(addr, &value);

  mt_gotoXY(FMT_TOP + 1, FMT_LEFT + 2);
  printf("dec: %05d | oct: %05o | hex: %04X | bin: ", value, value,
         value & 0xFFFF);

  for (int i = 14; i >= 0; i--)
    putchar(((value >> i) & 1) ? '1' : '0');
}

static void draw_cache(void) {
  bc_box(CACHE_TOP, CACHE_LEFT, CACHE_H, CACHE_W);
  title_center(CACHE_TOP, CACHE_LEFT, CACHE_W, "Кеш процессора", COLOR_GREEN);

  int base_rows[5] = {80, 10, 30, 50, 60};

  for (int r = 0; r < 5; r++) {
    mt_gotoXY(CACHE_TOP + 1 + r, CACHE_LEFT + 2);
    printf("%02d: ", base_rows[r]);

    for (int i = 0; i < 9; i++) {
      int addr = (base_rows[r] + i) % 100;
      printCell(addr, COLOR_WHITE, COLOR_BLACK);
    }
  }
}

static void draw_inout(int selected) {
  bc_box(INOUT_TOP, INOUT_LEFT, INOUT_H, INOUT_W);
  title_center(INOUT_TOP, INOUT_LEFT, INOUT_W, "IN--OUT", COLOR_GREEN);

  for (int i = 0; i < 5; i++) {
    mt_gotoXY(INOUT_TOP + 1 + i, INOUT_LEFT + 1);
    printTerm(i, i == selected);
  }
}

static void draw_keys(void) {
  bc_box(KEYS_TOP, KEYS_LEFT, KEYS_H, KEYS_W);
  title_center(KEYS_TOP, KEYS_LEFT, KEYS_W, "Клавиши", COLOR_GREEN);

  const int inner_w = KEYS_W - 2;
  const int text_col = KEYS_LEFT + 1;

  print_in_box(KEYS_TOP + 1, text_col, inner_w,
               "a/d - move   n - write demo   r - randomize");
  print_in_box(KEYS_TOP + 2, text_col, inner_w,
               "l - load     s - save         q - quit      i - reset");
}

static void draw_input_prompt(void) {
  mt_gotoXY(INPUT_TOP, INPUT_LEFT);
  mt_setfgcolor(COLOR_WHITE);
  fputs("Команда: a/d/n/r/l/s/q/i", stdout);
  mt_setdefaultcolor();
}

static void read_line(char *buf, size_t bufsz) {
  if (!buf || bufsz == 0)
    return;

  if (!fgets(buf, (int)bufsz, stdin)) {
    buf[0] = '\0';
    return;
  }
  buf[strcspn(buf, "\n")] = '\0';
}

static int hexval(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

/* Парсит "+FFFF" / "-FFFF" / "FFFF" (HEX).
   Возвращает value в формате: sign(bit14) + magnitude(14bit). */
static int parse_sc_value(const char *s, int *out) {
  if (!s || !out)
    return -1;

  while (*s == ' ' || *s == '\t')
    s++;

  int sign = 0;
  if (*s == '+' || *s == '-') {
    sign = (*s == '-');
    s++;
  }

  /* пропустим пробелы после знака */
  while (*s == ' ' || *s == '\t')
    s++;

  if (strlen(s) != 4)
    return -1;

  int mag = 0;
  for (int i = 0; i < 4; i++) {
    int hv = hexval((unsigned char)s[i]);
    if (hv < 0)
      return -1;
    mag = (mag << 4) | hv;
  }

  mag &= 0x3FFF; /* 14 бит */
  *out = (sign ? (1 << 14) : 0) | mag;
  return 0;
}

int main(void) {
  int rows, cols;

  setlocale(LC_ALL, "");

  if (!isatty(STDOUT_FILENO)) {
    fprintf(stderr, "Ошибка: stdout не является терминалом.\n");
    return 1;
  }

  if (mt_getscreensize(&rows, &cols) != 0) {
    fprintf(stderr, "Ошибка: не удалось определить размер терминала.\n");
    return 1;
  }

  if (rows < MIN_ROWS || cols < MIN_COLS) {
    fprintf(stderr,
            "Ошибка: маленький терминал (%dx%d). Нужно минимум %dx%d.\n", rows,
            cols, MIN_ROWS, MIN_COLS);
    return 1;
  }

  setvbuf(stdout, NULL, _IONBF, 0);

  sc_memoryInit();
  sc_regInit();
  sc_accumulatorInit();
  sc_icounterInit();

  /* демо */
  sc_memorySet(0, 0x3434);
  sc_memorySet(1, 0x1000);
  sc_memorySet(2, 0x1102);
  sc_memorySet(3, 0x1103);
  sc_accumulatorSet(0);
  sc_icounterSet(0x000E);

  int selected = 0;

  mt_setcursorvisible(0);

  rk_mytermsave();
  rk_mytermregime(1, 0, 1, 0, 0); /* noncanon, vtime=0 vmin=1, echo off */

  for (;;) {
    mt_clrscr();

    draw_memory(selected);
    draw_accumulator();
    draw_flags();
    draw_icounter();
    draw_command(selected);
    draw_bigcell(selected);

    draw_format(selected);
    draw_cache();
    draw_inout(selected);
    draw_keys();
    draw_input_prompt();

    enum keys key = KEY_UNKNOWN;
    if (rk_readkey(&key) != 0)
      continue;

    /* ESC — выход */
    if (key == KEY_ESC)
      break;

    /* Стрелки */
    if (key == KEY_RIGHT) {
      selected = (selected + 1) % MEM_CELLS_TOTAL;
      continue;
    }
    if (key == KEY_LEFT) {
      selected = (selected + MEM_CELLS_TOTAL - 1) % MEM_CELLS_TOTAL;
      continue;
    }
    if (key == KEY_UP) {
      if (selected >= MEM_COLS_PER_ROW)
        selected -= MEM_COLS_PER_ROW;
      continue;
    }
    if (key == KEY_DOWN) {
      if (selected + MEM_COLS_PER_ROW < MEM_CELLS_TOTAL)
        selected += MEM_COLS_PER_ROW;
      continue;
    }

    /* Буквенные команды: берём символ из rk_last_char */
    if (key == KEY_CHAR) {
      int ch = rk_last_char;

      if (ch >= 'A' && ch <= 'Z')
        ch = ch - 'A' + 'a'; /* нормализуем в lower */

      if (ch == 'q')
        break;

      if (ch == 'i') {
        sc_memoryInit();
        sc_accumulatorSet(0);
        sc_icounterSet(0);
        continue;
      }

      if (ch == 'n') {
        int newv = (selected * 37 + 0x123) & 0x3FFF;
        sc_memorySet(selected, newv);
        continue;
      }

      if (ch == 'r') {
        for (int i = 0; i < 20; i++) {
          int addr = (i * 7 + 13) % MEM_CELLS_TOTAL;
          int val = (addr * 91 + 0x2A) & 0x3FFF;
          sc_memorySet(addr, val);
        }
        continue;
      }

      if (ch == 's') {
        char filename[128];

        /* на время ввода файла: возвращаем канонический режим + echo */
        rk_mytermregime(0, 0, 0, 1, 1);

        mt_gotoXY(INPUT_TOP, INPUT_LEFT);
        mt_setfgcolor(COLOR_WHITE);
        fputs("Введите имя файла для сохранения: ", stdout);
        mt_setdefaultcolor();
        fflush(stdout);

        read_line(filename, sizeof(filename));
        if (filename[0] != '\0')
          sc_memorySave(filename);

        /* обратно в неканонический, без echo */
        rk_mytermregime(1, 0, 1, 0, 0);
        continue;
      }

      if (ch == 'l') {
        char filename[128];

        rk_mytermregime(0, 0, 0, 1, 1);

        mt_gotoXY(INPUT_TOP, INPUT_LEFT);
        mt_setfgcolor(COLOR_WHITE);
        fputs("Введите имя файла для загрузки: ", stdout);
        mt_setdefaultcolor();
        fflush(stdout);

        read_line(filename, sizeof(filename));
        if (filename[0] != '\0')
          sc_memoryLoad(filename);

        rk_mytermregime(1, 0, 1, 0, 0);
        continue;
      }
    }

    /* ENTER — редактирование выбранной ячейки памяти */
    if (key == KEY_ENTER) {
      int new_value;

      /* подсказка внизу */
      mt_gotoXY(INPUT_TOP, INPUT_LEFT);
      mt_setfgcolor(COLOR_WHITE);
      fputs("Введите значение ячейки (+FFFF/-FFFF/FFFF): ", stdout);
      mt_setdefaultcolor();
      fflush(stdout);

      /* rk_readvalue читает до Enter и возвращает value в формате SC */
      if (rk_readvalue(&new_value, -1) == 0) {
        sc_memorySet(selected, new_value);
      }
      continue;
    }

    /* F5 — редактирование аккумулятора */
    if (key == KEY_F5) {
      char buf[64];
      int new_value;

      /* временно включаем обычный ввод, чтобы было видно что печатаешь */
      rk_mytermregime(0, 0, 0, 1, 1); /* canonical + echo */

      mt_gotoXY(INPUT_TOP, INPUT_LEFT);
      mt_setfgcolor(COLOR_WHITE);
      fputs("Введите аккумулятор (+FFFF/-FFFF/FFFF): ", stdout);
      mt_setdefaultcolor();
      fflush(stdout);

      read_line(buf, sizeof(buf));

      /* обратно в неканонический */
      rk_mytermregime(1, 0, 1, 0, 0);

      if (parse_sc_value(buf, &new_value) == 0) {
        sc_accumulatorSet(new_value);
      }
      continue;
    }

    /* F6 — редактирование счётчика команд */
    if (key == KEY_F6) {
      int ic;

      mt_gotoXY(INPUT_TOP, INPUT_LEFT);
      mt_setfgcolor(COLOR_WHITE);
      fputs("Введите счётчик команд (0..127): ", stdout);
      mt_setdefaultcolor();
      fflush(stdout);

      /* Для IC удобнее читать обычное число в каноническом режиме */
      rk_mytermregime(0, 0, 0, 1, 1);

      char buf[32];
      read_line(buf, sizeof(buf));

      rk_mytermregime(1, 0, 1, 0, 0);

      ic = atoi(buf);
      if (ic >= 0 && ic < SC_MEMORY_SIZE) {
        sc_icounterSet(ic);
      }
      continue;
    }
  }

  rk_mytermrestore();

  mt_setdefaultcolor();
  mt_setcursorvisible(1);
  mt_gotoXY(rows, 1);
  putchar('\n');

  return 0;
}
