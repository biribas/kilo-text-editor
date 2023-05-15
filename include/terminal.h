#include <main.h>

#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED
  void enableRawMode(void);
  void disableRawMode(void);
  void die(const char *str);
  int editorReadKey(void);
  bool getCursorPosition(int *rows, int *cols);
  bool getWindowSize(int *rows, int *cols);
#endif
