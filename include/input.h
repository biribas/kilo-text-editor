#include <main.h>

#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED
  char *editorPrompt(char *, bool (*callback)(char *, int));
  void editorMoveCursor(int);
  void editorProcessKeypress(void);
#endif
