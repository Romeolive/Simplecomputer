#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "myReadKey.h"

int rk_last_char = 0;

static struct termios g_saved;
static int g_saved_ok = 0;

int
rk_mytermsave (void)
{
  if (tcgetattr (STDIN_FILENO, &g_saved) != 0)
    return -1;
  g_saved_ok = 1;
  return 0;
}

int
rk_mytermrestore (void)
{
  if (!g_saved_ok)
    return -1;
  if (tcsetattr (STDIN_FILENO, TCSANOW, &g_saved) != 0)
    return -1;
  return 0;
}

int
rk_mytermregime (int regime, int vtime, int vmin, int echo, int sigint)
{
  struct termios t;

  if (tcgetattr (STDIN_FILENO, &t) != 0)
    return -1;

  if (regime)
    t.c_lflag &= (tcflag_t)~ICANON;
  else
    t.c_lflag |= ICANON;

  if (echo)
    t.c_lflag |= ECHO;
  else
    t.c_lflag &= (tcflag_t)~ECHO;

  if (sigint)
    t.c_lflag |= ISIG;
  else
    t.c_lflag &= (tcflag_t)~ISIG;

  if (regime)
    {
      if (vtime < 0)
        vtime = 0;
      if (vmin < 0)
        vmin = 0;
      t.c_cc[VTIME] = (cc_t)vtime;
      t.c_cc[VMIN] = (cc_t)vmin;
    }

  if (tcsetattr (STDIN_FILENO, TCSANOW, &t) != 0)
    return -1;

  return 0;
}

static int
read_byte (unsigned char *ch)
{
  ssize_t n = read (STDIN_FILENO, ch, 1);
  if (n == 1)
    return 0;
  return -1;
}

static int
read_byte_timeout (unsigned char *ch, int timeout_ms)
{
  if (timeout_ms < 0)
    return read_byte (ch);

  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET (STDIN_FILENO, &rfds);

  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  int rc = select (STDIN_FILENO + 1, &rfds, NULL, NULL,
                   (timeout_ms == 0) ? &(struct timeval){ 0, 0 } : &tv);

  if (rc <= 0)
    return -1;
  return read_byte (ch);
}

int
rk_readkey (enum keys *key)
{
  unsigned char ch;

  if (key == NULL)
    return -1;
  *key = KEY_UNKNOWN;
  rk_last_char = 0;

  if (read_byte (&ch) != 0)
    return -1;

  if (ch == 27) /* ESC */
    {
      unsigned char seq[8];
      int n = 0;

      for (int i = 0; i < 7; i++)
        {
          unsigned char t;
          if (read_byte_timeout (&t, 20) != 0)
            break;
          seq[n++] = t;
        }

      if (n == 0)
        {
          *key = KEY_ESC;
          return 0;
        }

      if (n == 2 && seq[0] == '[')
        {
          if (seq[1] == 'A')
            {
              *key = KEY_UP;
              return 0;
            }
          if (seq[1] == 'B')
            {
              *key = KEY_DOWN;
              return 0;
            }
          if (seq[1] == 'C')
            {
              *key = KEY_RIGHT;
              return 0;
            }
          if (seq[1] == 'D')
            {
              *key = KEY_LEFT;
              return 0;
            }
        }

      if (n == 4 && seq[0] == '[' && seq[3] == '~')
        {
          if (seq[1] == '1' && seq[2] == '5')
            {
              *key = KEY_F5;
              return 0;
            }
          if (seq[1] == '1' && seq[2] == '7')
            {
              *key = KEY_F6;
              return 0;
            }
        }

      *key = KEY_UNKNOWN;
      return 0;
    }

  if (ch == '\n' || ch == '\r')
    {
      *key = KEY_ENTER;
      return 0;
    }

  *key = KEY_CHAR;
  rk_last_char = (int)ch;
  return 0;
}

static int
hexval (int c)
{
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return 10 + (c - 'a');
  if ('A' <= c && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

int
rk_readvalue (int *value, int timeout_ms)
{
  if (value == NULL)
    return -1;

  char buf[16];
  int pos = 0;

  for (;;)
    {
      unsigned char ch;
      if (read_byte_timeout (&ch, timeout_ms) != 0)
        return -1;

      if (ch == '\r' || ch == '\n')
        break;

      if (ch == 127 || ch == 8)
        {
          if (pos > 0)
            pos--;
          continue;
        }

      if (pos >= (int)sizeof (buf) - 1)
        continue;

      if (ch == '+' || ch == '-')
        {
          if (pos != 0)
            continue; /* знак только первым */
          buf[pos++] = (char)ch;
          continue;
        }

      if (hexval (ch) >= 0)
        {
          buf[pos++] = (char)ch;
          continue;
        }
    }

  buf[pos] = '\0';

  int sign = 0;
  const char *p = buf;
  if (buf[0] == '+' || buf[0] == '-')
    {
      sign = (buf[0] == '-');
      p = buf + 1;
    }

  if ((int)strlen (p) != 4)
    return -1;

  int mag = 0;
  for (int i = 0; i < 4; i++)
    {
      int hv = hexval ((unsigned char)p[i]);
      if (hv < 0)
        return -1;
      mag = (mag << 4) | hv;
    }

  mag &= 0x3FFF;
  *value = (sign ? (1 << 14) : 0) | mag;
  return 0;
}
