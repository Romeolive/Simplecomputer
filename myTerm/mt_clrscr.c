#include <sys/ioctl.h>
#include <unistd.h>

#include "myTerm.h"
#include <stdio.h>

static int write_str(const char *s) {
  size_t len = 0;

  if (s == NULL) {
    return -1;
  }

  while (s[len] != '\0') {
    len++;
  }

  if (write(STDOUT_FILENO, s, len) < 0) {
    return -1;
  }

  return 0;
}

int mt_clrscr(void) {
  /* clear screen + cursor home */
  if (write_str("\033[H\033[2J") != 0) {
    return -1;
  }
  return 0;
}

int mt_gotoXY(int row, int col) {
  char buf[32];
  int n;

  if (row < 1 || col < 1) {
    return -1;
  }

  n = snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
  if (n <= 0 || n >= (int)sizeof(buf)) {
    return -1;
  }

  return write_str(buf);
}

int mt_getscreensize(int *rows, int *cols) {
  struct winsize ws;

  if (rows == NULL || cols == NULL) {
    return -1;
  }

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
    return -1;
  }

  if (ws.ws_row == 0 || ws.ws_col == 0) {
    return -1;
  }

  *rows = (int)ws.ws_row;
  *cols = (int)ws.ws_col;

  return 0;
}

int mt_setfgcolor(enum colors color) {
  char buf[16];
  int n;

  if (color < COLOR_BLACK || color > COLOR_WHITE) {
    return -1;
  }

  n = snprintf(buf, sizeof(buf), "\033[3%dm", (int)color);
  if (n <= 0 || n >= (int)sizeof(buf)) {
    return -1;
  }

  return write_str(buf);
}

int mt_setbgcolor(enum colors color) {
  char buf[16];
  int n;

  if (color < COLOR_BLACK || color > COLOR_WHITE) {
    return -1;
  }

  n = snprintf(buf, sizeof(buf), "\033[4%dm", (int)color);
  if (n <= 0 || n >= (int)sizeof(buf)) {
    return -1;
  }

  return write_str(buf);
}

int mt_setdefaultcolor(void) { return write_str("\033[0m"); }

int mt_setcursorvisible(int value) {
  if (value) {
    return write_str("\033[?25h");
  }
  return write_str("\033[?25l");
}

int mt_delline(void) {
  /* delete/clear current line */
  return write_str("\033[2K");
}
