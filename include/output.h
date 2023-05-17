#include <main.h>

#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED
  void editorScrollX(void);
  void editorScrollY(void);
  void editorHighlightOutput(buffer *, color_t color);
  void editorDrawLines(buffer *);
  void setDefaultColors(buffer *);
  void printTextLine(int row, color_t background, buffer *);
  void printMainScreen(buffer *);
  void printLineNumber(int row, buffer *);
  void adjustSidebarWidth(void);
  void editorDrawStatusBar(buffer *);
  void editorDrawMessageBar(buffer *);
  void editorRefreshScreen(void);
  void editorSetStatusMessage(const char *format, ...);
#endif
