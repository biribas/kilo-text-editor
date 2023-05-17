#include <highlight.h>
#include <lines.h>
#include <output.h>

int editorLineCxToRx(editorLine *line, int cursorX) {
  int rx = 0;
  for (int i = 0; i < cursorX; i++) {
    if (line->content[i] == '\t')
      rx += TAB_SIZE - (rx % TAB_SIZE);
    else
      rx++;
  }
  return rx;
}

int editorLineRxToCx(editorLine *line, int rCursorX) {
  int rx = 0;
  int cx;

  for (cx = 0; cx < line->length; cx++) {
    if (line->content[cx] == '\t')
      rx += TAB_SIZE - (rx % TAB_SIZE);
    else 
      rx++;

    if (rx > rCursorX)
      return cx;
  }

  return cx;
}

void editorUpdateLine(editorLine *line) {
  int tabs = 0;
  for (int i = 0; i < line->length; i++) {
    if (line->content[i] == '\t')
      tabs++;
  }
  
  free(line->renderContent);
  line->renderContent = malloc(line->length + (TAB_SIZE - 1) * tabs + 1);

  int index = 0;
  for (int i = 0; i < line->length; i++) {
    if (line->content[i] == '\t') {
      line->renderContent[index++] = ' ';
      while (index % TAB_SIZE != 0)
        line->renderContent[index++] = ' ';
    }
    else {
      line->renderContent[index++] = line->content[i];
    }
  }

  line->renderContent[index] = '\0';
  line->renderLength = index;

  editorUpdateHighlight(line);
}

void editorInsertLine(int at, char *line, size_t length) {
  if (at < 0 || at > E.numlines)
    return;

  E.lines = realloc(E.lines, sizeof(editorLine) * (E.numlines + 1));
  memmove(&E.lines[at + 1], &E.lines[at], sizeof(editorLine) * (E.numlines - at));

  for (int i = at + 1; i <= E.numlines; i++)
    E.lines[i].index++;

  E.lines[at].index = at;

  E.lines[at].length = length;
  E.lines[at].content = malloc(length + 1);
  memcpy(E.lines[at].content, line, length);
  E.lines[at].content[length] = '\0';

  E.lines[at].highlight = NULL;
  E.lines[at].renderContent = NULL;
  E.lines[at].renderLength = 0;
  E.lines[at].isOpenComment = false;

  editorUpdateLine(&E.lines[at]);
  E.numlines++;

  adjustSidebarWidth();
}

void editorFreeLine(editorLine *line) {
  free(line->content);
  free(line->renderContent);
  free(line->highlight);
}

void editorDeleteLine(int at) {
  if (at < 0 || at >= E.numlines)
    return;

  editorFreeLine(&E.lines[at]);
  memmove(&E.lines[at], &E.lines[at + 1], sizeof(editorLine) * (E.numlines - at - 1));

  for (int i = at; i < E.numlines - 1; i++)
    E.lines[i].index--;

  E.numlines--;

  adjustSidebarWidth();
}

void editorLineInsertChar(editorLine *line, int at, int c) {
  if (at < 0 || at > line->length) 
    at = line->length;

  line->content = realloc(line->content, line->length + 2);
  memmove(&line->content[at + 1], &line->content[at], line->length - at + 1);
  line->length++;
  line->content[at] = c;
  editorUpdateLine(line);
}

void editorLineAppendString(editorLine *line, char *s, size_t len) {
  line->content = realloc(line->content, line->length + len + 1);
  memcpy(&line->content[line->length], s, len);
  line->length += len;
  line->content[line->length] = '\0';
  editorUpdateLine(line);
}

void editorLineDeleteChar(editorLine *line, int at) {
  if (at < 0 || at > line->length)
    return;

  memmove(&line->content[at], &line->content[at + 1], line->length - at);
  line->length--; 
  editorUpdateLine(line);
}

void freeMemory(void) {
  free(E.filename);
  for (int i = 0; i < E.numlines; i++) {
    editorFreeLine(&E.lines[i]);
  }
  free(E.lines);
}

