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
    else if (c == ESC) {
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
    else if (c == RETURN) {
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
        E.cursorX = max(0, E.lines[E.cursorY].length + (E.mode == NORMAL ? -1 : 0));;
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

  fixCursorXPosition();
}

void editorProcessKeypress(void) {
  static int quit_times = QUIT_TIMES;

  int c = editorReadKey();

  switch (c) {
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      return;

    case HOME_KEY:
      E.cursorX = 0;
      return;

    case PAGE_UP:
    case PAGE_DOWN: {
      if (E.numlines == 0) return;

      E.cursorY = c == PAGE_UP
        ? E.rowOffset
        : min(E.rowOffset + E.screenRows, E.numlines) - 1;

      int times = E.screenRows;
      int direction = c == PAGE_UP ? ARROW_UP : ARROW_DOWN;
      while (times--)
        editorMoveCursor(direction);
      return;
    }

    case CTRL_KEY('q'):
      quit_times--;
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("File has unsaved changes. Press Ctrl-Q again to quit");
        return;
      }
      write(STDOUT_FILENO, "\x1b[H", 3);  // Moves cursor to home position (0, 0)
      system(CLEAR);
      exit(0);
      return;

    case CTRL_KEY('s'):
      editorSave();
      return;

    case CTRL_KEY('f'):
      editorFind();
      return;

    default:
      break;
  }

  if (quit_times < QUIT_TIMES) {
    E.isPromptOpen = false;
    quit_times = QUIT_TIMES;
  }

  if (E.mode == NORMAL) {
    handleNormalMode(c);
  }
  else if (E.mode == INSERT) {
    handleInsertMode(c);
  }
}

