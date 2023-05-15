#include <main.h>

#ifndef HIGHLIGHT_H_INCLUDED
#define HIGHLIGHT_H_INCLUDED
  bool isSeparator(int);
  bool colorcmp(color_t, color_t);
  void colorLine(editorLine *line, int start, color_t, int len);
  bool highlightSinglelineComments(editorLine *line, highlightController *);
  bool highlightMultilineComments(editorLine *line, highlightController *);
  bool highlightStrings(editorLine *line, highlightController *);
  bool highlightNumbers(editorLine *line, highlightController *);
  bool highlightKeywords(editorLine *line, highlightController *);
  bool highlightOperators(editorLine *line, highlightController *);
  bool highlightSymbols(editorLine *line, highlightController *, char **symbols, color_t);
  void editorUpdateHighlight(editorLine *line);
  void editorSelectSyntaxHighlight(void);
  void clearSearchHighlight(void);
#endif
