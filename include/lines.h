#include <main.h>

#ifndef LINES_H_INCLUDED
#define LINES_H_INCLUDED
  int indentation(editorLine *line);
  int editorLineCxToRx(editorLine *line, int cursorX);
  int editorLineRxToCx(editorLine *line, int rCursorX);
  void editorUpdateLine(editorLine *);
  void editorInsertLine(int at, char *line, size_t length);
  void editorFreeLine(editorLine *line);
  void editorDeleteLine(int at);
  void editorLineInsertChar(editorLine *line, int at, int c);
  void editorLineAppendString(editorLine *line, char *string, size_t len);
  void editorLineDeleteChar(editorLine *line, int at);
  void freeMemory(void);
#endif
