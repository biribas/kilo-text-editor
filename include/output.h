#include <main.h>

#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED
  void editorScrollX(void);
  void editorScrollY(void);
  void editorHighlightOutput(buffer *buff, color_t color);
  void editorDrawLines(buffer *);
  void printLineNumber(int row, buffer *buff);
  void adjustSidebarWidth(void);
  void editorDrawStatusBar(buffer *);
  void editorDrawMessageBar(buffer *);
  void editorRefreshScreen(void);
  void editorSetStatusMessage(const char *format, ...);
#endif
