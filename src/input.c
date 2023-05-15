#include <editor.h>
#include <fileio.h>
#include <finder.h>
#include <input.h>
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
  buf[0] = '\0';

  while (true) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

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

      editorSetStatusMessage("");
      free(buf);
      return NULL;
    }
    else if (c == '\r') {
      if (buflen == 0) continue;
      if (callback) callback(buf, c);
      editorSetStatusMessage("");
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

void editorMoveCursor(int key) {
  if (E.numlines == 0) return;
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
      if (E.cursorX != 0) {
        E.cursorX--;
      }
      else if (E.cursorY > 0) {
        E.cursorY--;
        E.cursorX = E.lines[E.cursorY].length;
      }
      E.highestLastX = E.cursorX;
      break;

    case ARROW_RIGHT:
      if (E.cursorX < currentLine->length) {
        E.cursorX++;
      }
      else if (E.cursorY + 1 < E.numlines) {
        E.cursorY++;
        E.cursorX = 0;
      }
      E.highestLastX = E.cursorX;
      break;
  }

  currentLine = &E.lines[E.cursorY];
  E.cursorX = min(E.cursorX, currentLine->length);
}

void editorProcessKeypress(void) {
  static int quit_times = 2;

  int c = editorReadKey();

  switch (c) {
    case '\r':
      editorInsertNewLine();
      break;
    
    case CTRL_KEY('q'):
      quit_times--;
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("File has unsaved changes. Press Ctrl-Q again to quit");
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4); // Erase entire screen
      write(STDOUT_FILENO, "\x1b[H", 3);  // Moves cursor to home position (0, 0)
      exit(0);
      break;

    case CTRL_KEY('s'):
      editorSave();
      break;

    case CTRL_KEY('f'):
      editorFind();
      break;

    case HOME_KEY:
      E.cursorX = 0;
      break;

    case END_KEY:
      if (E.cursorY < E.numlines)
        E.cursorX = E.lines[E.cursorY].length;
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY: {
      if (c == DEL_KEY) {
        editorMoveCursor(ARROW_RIGHT);
      }
      editorDeleteChar();
      break;
    }

    case PAGE_UP:
    case PAGE_DOWN: {
      if (E.numlines == 0) break;

      E.cursorY = c == PAGE_UP
        ? E.rowOffset
        : min(E.rowOffset + E.screenRows, E.numlines) - 1;

      int times = E.screenRows;
      int direction = c == PAGE_UP ? ARROW_UP : ARROW_DOWN;
      while (times--)
        editorMoveCursor(direction);
      break;
    }

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;
    
    default:
      if (!iscntrl(c))
        editorInsertChar(c);
      break;
  }

  if (quit_times < QUIT_TIMES) {
    editorSetStatusMessage("");
    quit_times = QUIT_TIMES;
  }
}

