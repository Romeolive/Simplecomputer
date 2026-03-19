#ifndef MY_READKEY_H
#define MY_READKEY_H

#ifdef __cplusplus
extern "C"
{
#endif

  /* Логические клавиши */
  enum keys
  {
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

  int rk_mytermsave (void);
  int rk_mytermrestore (void);

  int rk_mytermregime (int regime, int vtime, int vmin, int echo, int sigint);

  /* Считывает клавишу (стрелки, F5/F6, Enter, ESC, обычные символы) */
  int rk_readkey (enum keys *key);

  int rk_readvalue (int *value, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
