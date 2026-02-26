#include <stdio.h>

#include "myBigChars.h"
#include "myTerm.h"

static void
cell (int row, int col, char c)
{
  mt_gotoXY (row, col);
  putchar (c);
}

int
bc_box (int top, int left, int height, int width)
{
  if (height < 2 || width < 2)
    return -1;

  /* top */
  cell (top, left, '+');
  for (int i = 1; i < width - 1; i++)
    cell (top, left + i, '-');
  cell (top, left + width - 1, '+');

  /* sides */
  for (int r = 1; r < height - 1; r++)
    {
      cell (top + r, left, '|');
      cell (top + r, left + width - 1, '|');
    }

  /* bottom */
  cell (top + height - 1, left, '+');
  for (int i = 1; i < width - 1; i++)
    cell (top + height - 1, left + i, '-');
  cell (top + height - 1, left + width - 1, '+');

  return 0;
}

/* 8x8 шаблоны. Символы рисуем символом '█' (можно заменить на '#') */
static void
draw8 (const char pattern[8][9], int row, int col)
{
  for (int r = 0; r < 8; r++)
    {
      mt_gotoXY (row + r, col);
      for (int c = 0; c < 8; c++)
        {
          putchar (pattern[r][c] == '1' ? '#' : ' ');
        }
    }
}

/* Минимальный набор: + - 0..9 A..F */
static const char P_PLUS[8][9] = {
  "00011000", "00011000", "00011000", "11111111",
  "11111111", "00011000", "00011000", "00011000",
};

static const char P_MINUS[8][9] = {
  "00000000", "00000000", "00000000", "11111111",
  "11111111", "00000000", "00000000", "00000000",
};

static const char P_0[8][9] = {
  "00111100", "01100110", "11000011", "11000011",
  "11000011", "11000011", "01100110", "00111100",
};

static const char P_1[8][9] = {
  "00011000", "00111000", "00011000", "00011000",
  "00011000", "00011000", "00011000", "01111110",
};

static const char P_2[8][9] = {
  "00111100", "01100110", "00000110", "00001100",
  "00011000", "00110000", "01100000", "01111110",
};

static const char P_3[8][9] = {
  "00111100", "01100110", "00000110", "00011100",
  "00011100", "00000110", "01100110", "00111100",
};

static const char P_4[8][9] = {
  "00001100", "00011100", "00101100", "01001100",
  "11111110", "00001100", "00001100", "00011110",
};

static const char P_5[8][9] = {
  "01111110", "01100000", "01100000", "01111100",
  "00000110", "00000110", "01100110", "00111100",
};

static const char P_6[8][9] = {
  "00111100", "01100110", "01100000", "01111100",
  "01100110", "01100110", "01100110", "00111100",
};

static const char P_7[8][9] = {
  "01111110", "00000110", "00001100", "00011000",
  "00110000", "00110000", "00110000", "00110000",
};

static const char P_8[8][9] = {
  "00111100", "01100110", "01100110", "00111100",
  "00111100", "01100110", "01100110", "00111100",
};

static const char P_9[8][9] = {
  "00111100", "01100110", "01100110", "01100110",
  "00111110", "00000110", "01100110", "00111100",
};

static const char P_A[8][9] = {
  "00011000", "00111100", "01100110", "01100110",
  "01111110", "01100110", "01100110", "01100110",
};

static const char P_B[8][9] = {
  "01111100", "01100110", "01100110", "01111100",
  "01111100", "01100110", "01100110", "01111100",
};

static const char P_C[8][9] = {
  "00111100", "01100110", "01100000", "01100000",
  "01100000", "01100000", "01100110", "00111100",
};

static const char P_D[8][9] = {
  "01111000", "01101100", "01100110", "01100110",
  "01100110", "01100110", "01101100", "01111000",
};

static const char P_E[8][9] = {
  "01111110", "01100000", "01100000", "01111100",
  "01111100", "01100000", "01100000", "01111110",
};

static const char P_F[8][9] = {
  "01111110", "01100000", "01100000", "01111100",
  "01111100", "01100000", "01100000", "01100000",
};

static const char (*pick (char ch))[9]
{
  switch (ch)
    {
    case '+':
      return P_PLUS;
    case '-':
      return P_MINUS;
    case '0':
      return P_0;
    case '1':
      return P_1;
    case '2':
      return P_2;
    case '3':
      return P_3;
    case '4':
      return P_4;
    case '5':
      return P_5;
    case '6':
      return P_6;
    case '7':
      return P_7;
    case '8':
      return P_8;
    case '9':
      return P_9;
    case 'A':
    case 'a':
      return P_A;
    case 'B':
    case 'b':
      return P_B;
    case 'C':
    case 'c':
      return P_C;
    case 'D':
    case 'd':
      return P_D;
    case 'E':
    case 'e':
      return P_E;
    case 'F':
    case 'f':
      return P_F;
    default:
      return P_0; /* fallback */
    }
}

int
bc_printbigchar (char ch, int row, int col, enum colors fg, enum colors bg)
{
  mt_setfgcolor (fg);
  mt_setbgcolor (bg);

  draw8 (pick (ch), row, col);

  mt_setdefaultcolor ();
  return 0;
}
