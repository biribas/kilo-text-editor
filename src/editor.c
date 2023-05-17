#include <editor.h>
#include <lines.h>

void editorInsertChar(int c) {
  editorLineInsertChar(&E.lines[E.cursorY], E.cursorX, c);
  E.cursorX++;
  E.highestLastX = E.cursorX;
  E.dirty = true;
}

void editorDeleteChar(void) {
  if (E.numlines == 0) return;
  if (E.cursorY == E.numlines) return;
  if (E.cursorX == 0 && E.cursorY == 0) return;

  editorLine *line = &E.lines[E.cursorY];
  if (E.cursorX > 0) {
    editorLineDeleteChar(line, --E.cursorX);
  }
  else {
    E.cursorX = E.lines[E.cursorY - 1].length;
    editorLineAppendString(&E.lines[E.cursorY - 1], line->content, line->length);
    editorDeleteLine(E.cursorY);
    E.cursorY--;
  }

  E.highestLastX = E.cursorX;
  E.dirty = true;
}

void editorInsertNewLine(void) {
  if (E.cursorX == 0) {
    editorInsertLine(E.cursorY, "", 0);
  }
  else {
    editorLine *line = &E.lines[E.cursorY];
    editorInsertLine(E.cursorY + 1, &line->content[E.cursorX], line->length - E.cursorX);
 
    line = &E.lines[E.cursorY];
    line->length = E.cursorX; 
    line->content = realloc(line->content, line->length + 1);
    line->content[line->length] = '\0';

    editorUpdateLine(line);
    E.cursorX = 0;
  }

  E.highestLastX = E.cursorX;
  E.cursorY++;
  E.dirty = true;
}

