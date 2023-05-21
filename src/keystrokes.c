#include <editor.h>
#include <fileio.h>
#include <finder.h>
#include <input.h>
#include <keystrokes.h>
#include <lines.h>
#include <output.h>
#include <terminal.h>
#include <tools.h>

void handleNormalMode(int c) {
  switch (c) {
    // Insert before the cursor
    case 'i':
      E.mode = INSERT;
      break;

    // Insert at the beginning of the line
    case 'I': {
      E.mode = INSERT;
      E.cursorX = indentation(&E.lines[E.cursorY]);
      break;
    }

    // Insert after the cursor
    case 'a':
      E.mode = INSERT;
      E.cursorX++;
      break;

    // Insert at the end of the line
    case 'A':
      E.mode = INSERT;
      E.cursorX = E.lines[E.cursorY].length;
      break;

    case 'o': // Append a new line bellow the current line
    case 'O': // Append a new line above the current line
    {
      int tabs = indentation(&E.lines[E.cursorY]);
      char line[tabs];
      memset(line, '\t', tabs);
      editorInsertLine(c == 'o' ? ++E.cursorY : E.cursorY, line, tabs);

      E.mode = INSERT;
      E.cursorX = tabs;
      break;
    }

    case 'g': {
      switch (editorReadKey()) {
        // Go to the first line of the document
        case 'g':
          E.cursorY = 0;
          break;

        default:
          break;
      }

      break;
    }

    // Go to the last line of the document
    case 'G':
      E.cursorY = E.numlines - 1;
      break;

    // Jump to next paragraph
    case '}':
      for (int i = E.cursorY + 1; i < E.numlines; i++) {
        if (E.lines[i].length != 0) continue;
        E.cursorY = i;
        return;
      }
      E.cursorY = E.numlines - 1;
      break; 

    // Jump to previus paragraph
    case '{':
      for (int i = E.cursorY - 1; i >= 0; i--) {
        if (E.lines[i].length != 0) continue;
        E.cursorY = i;
        return;
      }
      E.cursorY = 0;
      break;

    // Arrow keys
    case 'h':
    case BACKSPACE:
    case ARROW_LEFT:
      editorMoveCursor(ARROW_LEFT);
      break;

    case 'j':
    case ARROW_DOWN:
      editorMoveCursor(ARROW_DOWN);
      break;

    case 'k':
    case ARROW_UP:
      editorMoveCursor(ARROW_UP);
      break;

    case 'l':
    case ' ':
    case ARROW_RIGHT:
      editorMoveCursor(ARROW_RIGHT);
      break;

    case HOME_KEY:
    case '0':
      E.cursorX = 0;
      break;

    case END_KEY:
    case '$':
      E.cursorX = E.lines[E.cursorY].length - 1;
      break;
  }
}

void handleInsertMode(int c) {
  static int quit_times = QUIT_TIMES;

  switch (c) {
    case '\x1b':
    case CTRL_KEY('c'):
      E.mode = NORMAL;
      if (E.cursorX != 0)
        E.cursorX--;
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

