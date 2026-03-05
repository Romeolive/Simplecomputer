#ifndef MY_READKEY_H
#define MY_READKEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Логические клавиши */
enum keys {
  KEY_UNKNOWN = 0,

  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,

  KEY_ENTER,
  KEY_ESC,

  KEY_F5,
  KEY_F6,

  /* Для обычных символов */
  KEY_CHAR
};

/* Если key == KEY_CHAR, то сюда кладём сам символ */
extern int rk_last_char;

int rk_mytermsave(void);
int rk_mytermrestore(void);

/* regime: 0 — canonical, 1 — noncanonical
   vtime/vmin — как в termios (используется в некон. режиме)
   echo: 0/1
   sigint: 0/1 (вкл/выкл ISIG)
*/
int rk_mytermregime(int regime, int vtime, int vmin, int echo, int sigint);

/* Считывает клавишу (стрелки, F5/F6, Enter, ESC, обычные символы) */
int rk_readkey(enum keys *key);

/* Ввод значения ячейки.
   Принимает: +XXXX, -XXXX или XXXX (HEX, 4 символа).
   Возвращает value в формате SimpleComputer (sign в бите 14, magnitude 14 бит).
   timeout (ms): -1 блокирующий, 0 не ждать, >0 ждать указанное время.
*/
int rk_readvalue(int *value, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
