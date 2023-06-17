#include <finder.h>
#include <highlight.h>
#include <input.h>
#include <lines.h>
#include <output.h>
#include <tools.h>

char *findLastOccurrence(char *lasMatch, char *query, char *lineContent) {
  char *prev = NULL;
  char *cur = lineContent;

  while (true) {
    cur = strstr(cur, query);
    if (cur == lasMatch) break;
    prev = cur;
    cur++;
  }
  return prev;
}

bool editorFindCallback(char *query, int key) {
  static int current;
  static int direction;
  static char *match = NULL;

  clearSearchHighlight();

  if (E.numlines == 0 || *query == '\0' || key == RETURN || key == ESC) {
    return false;
  }

  if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1; 
  }
  else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1; 
  }
  else {
    current = E.savedLastY;
    direction = 1;
    match = E.lines[current].renderContent + E.savedLastX;
  }

  int lastMatchIndex;
  char *lastMatch;

  int limit = current;
  bool found = false;

  while (true) {
    editorLine *line = &E.lines[current];

    // Same line
    if (match) {
      match = direction == -1
        ? findLastOccurrence(match, query, line->renderContent)
        : strstr(match + 1, query);
    }

    // Next Line
    if (!match) {
      // Highlight case
      if (found && current + direction == limit) break;

      // Search case
      int nextLine = mod(current + direction, E.numlines);
      current = nextLine;
      line = &E.lines[current];

      match = direction == -1
        ? findLastOccurrence(match, query, line->renderContent)
        : strstr(line->renderContent, query);

      if (!match && current == limit) break;
    }

    if (match) {
      if (!found) {
        E.cursorY = current;
        E.cursorX = editorLineRxToCx(line, match - line->renderContent);

        E.rowOffset = E.screenRows <= E.cursorY
          ? E.cursorY - E.screenRows / 2
          : E.numlines;

        editorScrollY();

        found = true;

        lastMatchIndex = current;
        lastMatch = match;
        direction = 1;

        current = E.rowOffset;
        limit = min(E.rowOffset + E.screenRows, E.numlines);

        match = E.lines[current].renderContent - 1;
      }
      else {
        colorLine(line, match - line->renderContent, theme.match.unselected, strlen(query));
      }
    }
  }

  if (found) {
    current = lastMatchIndex;
    match = lastMatch;
    editorLine *line = &E.lines[current];
    colorLine(line, match - line->renderContent, theme.match.selected, strlen(query));
  }

  return found;
}

void editorFind(void) {
  char *query = editorPrompt("Search: %s", editorFindCallback);
  free(query);
}

