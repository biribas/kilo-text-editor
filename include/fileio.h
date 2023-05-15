#include <main.h>

#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED
  char *editorLinesToString(int *buflen);
  void editorOpen(char *filename);
  void editorSave(void);
#endif
