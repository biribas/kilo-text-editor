#include <main.h>

#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED
  void editorRefreshScreen(void);
  void editorScrollX(void);
  void editorScrollY(void);
  void editorDrawLines(buffer *);
  void editorDrawStatusBar(buffer *);
  void editorDrawPromptBar(buffer *);
  void editorSetCursorPosition(buffer *);

  void editorSetStatusMessage(const char *format, ...);
  void editorHighlightOutput(buffer *, color_t color);
  void setDefaultColors(buffer *);
  void printLineNumber(int row, buffer *);
  void printTextLine(int row, color_t background, buffer *);
  void printMainScreen(buffer *);
  void adjustSidebarWidth(void);
#endif
