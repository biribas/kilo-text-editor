#include <editor.h>
#include <fileio.h>
#include <finder.h>
#include <input.h>
#include <keystrokes.h>
#include <output.h>
#include <terminal.h>
#include <tools.h>

char *editorPrompt(char *prompt, bool (*callback)(char *, int)) {
  // Used only in search
  E.savedLastX = E.cursorX;
  E.savedLastY = E.cursorY;
  int saved_rowOff = E.rowOffset;
  int saved_colOff = E.colOffset;

  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  *buf = '\0';

  while (true) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();
    refreshPromptCursor();

    int c = editorReadKey();

    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen == 0) continue;
      buf[--buflen] = '\0';
    }
    else if (c == '\x1b') {
      if (callback) {
        callback(buf, c);
        E.cursorX = E.savedLastX;
        E.cursorY = E.savedLastY;
        E.rowOffset = saved_rowOff;
        E.colOffset = saved_colOff;
      }
      free(buf);

      E.isPromptOpen = false;
      return NULL;
    }
    else if (c == '\r') {
      if (buflen == 0) continue;
      if (callback) callback(buf, c);

      E.isPromptOpen = false;
      return buf;
    }
    else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback && !callback(buf, c)) {
      E.cursorX = E.savedLastX;
      E.cursorY = E.savedLastY;
      E.rowOffset = saved_rowOff;
      E.colOffset = saved_colOff;
    }
  }
}

void refreshPromptCursor(void) {
  char temp[32];
  snprintf(temp, sizeof(temp), "\x1b[%d;%luH", E.rowOffset + E.screenRows + 2, strlen(E.statusmsg) + 1);
  write(STDOUT_FILENO, temp, strlen(temp));
}

void editorMoveCursor(int key) {
  editorLine *currentLine = &E.lines[E.cursorY];

  switch (key) {
    case ARROW_UP:
      if (E.cursorY != 0) {
        E.cursorY--;
        E.cursorX = E.highestLastX;
      }
      break;

    case ARROW_DOWN:
      if (E.cursorY + 1 < E.numlines) {
        E.cursorY++;
        E.cursorX = E.highestLastX;
      }
      break;

    case ARROW_LEFT:
      if (E.cursorX > 0) {
        E.cursorX--;
      }
      else if (E.cursorY > 0) {
        E.cursorY--;
        int rightLimit = max(0, E.lines[E.cursorY].length + (E.mode == NORMAL ? -1 : 0));
        E.cursorX = rightLimit;
      }
      E.highestLastX = E.cursorX;
      break;

    case ARROW_RIGHT: {
      int cursorLimit = currentLine->length + (E.mode == NORMAL ? -1 : 0);
      if (E.cursorX < cursorLimit) {
        E.cursorX++;
      }
      else if (E.cursorY + 1 < E.numlines) {
        E.cursorY++;
        E.cursorX = 0;
      }
      E.highestLastX = E.cursorX;
      break;
    }
  }

  currentLine = &E.lines[E.cursorY];
  E.cursorX = min(E.cursorX, currentLine->length);
}

void editorProcessKeypress(void) {
  int c = editorReadKey();

  if (E.mode == NORMAL) {
    handleNormalMode(c);
  }
  else if (E.mode == INSERT) {
    handleInsertMode(c);
  }
}

