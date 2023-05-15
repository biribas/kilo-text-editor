#include <main.h>

#ifndef FINDER_H_INCLUDED
#define FINDER_H_INCLUDED
  char *findLastOccurrence(char *lasMatch, char *query, char *lineContent);
  bool editorFindCallback(char *query, int key);
  void editorFind(void);
#endif
