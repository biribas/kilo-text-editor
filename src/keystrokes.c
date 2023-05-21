#include <editor.h>
#include <fileio.h>
#include <finder.h>
#include <input.h>
#include <keystrokes.h>
#include <output.h>
#include <tools.h>

void handleNormalMode(int c) {
  switch (c) {
    case 'i':
      E.mode = INSERT;
      break;

    case 'a':
      E.cursorX++;
      E.mode = INSERT;
      break;

    case 'h':
      editorMoveCursor(ARROW_LEFT);
      break;
    case 'j':
      editorMoveCursor(ARROW_DOWN);
      break;
    case 'k':
      editorMoveCursor(ARROW_UP);
      break;
    case 'l':
      editorMoveCursor(ARROW_RIGHT);
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
  }
}

void handleInsertMode(int c) {
  static int quit_times = QUIT_TIMES;

  switch (c) {
    case '\x1b':
    case CTRL_KEY('c'):
      if (E.cursorX != 0) {
        E.cursorX--;
      }
      E.mode = NORMAL;
      break;

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
    
    default:
      if (c == '\t' || !iscntrl(c))
        editorInsertChar(c);
      break;
  }

  if (quit_times < QUIT_TIMES) {
    E.isPromptOpen = false;
    quit_times = QUIT_TIMES;
  }
}

