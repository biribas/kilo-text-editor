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
      if (E.cursorX != 0) {
        E.cursorX++;
      }
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

    case 'x': // delete character
    case 's': // delete character and substitute text
      if (E.lines[E.cursorY].length == 0) break;
      if (c == 's') E.mode = INSERT;
      editorLineDeleteChar(&E.lines[E.cursorY], E.cursorX);
      break;

    case 'g': {
      switch (editorReadKey()) {
        // Go to the first line of the document
        case 'g':
          E.cursorY = 0;
          E.cursorX = clamp(0, E.cursorX, E.lines[E.cursorY].length - 1);
          break;

        default:
          break;
      }

      break;
    }

    // Go to the last line of the document
    case 'G':
      E.cursorY = E.numlines - 1;
      E.cursorX = clamp(0, E.cursorX, E.lines[E.cursorY].length - 1);
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
      editorMoveCursor(ARROW_LEFT);
      break;

    case 'j':
      editorMoveCursor(ARROW_DOWN);
      break;

    case 'k':
      editorMoveCursor(ARROW_UP);
      break;

    case 'l':
    case ' ':
      editorMoveCursor(ARROW_RIGHT);
      break;

    case '0':
      E.cursorX = 0;
      break;

    case END_KEY:
    case '$':
      E.cursorX = max(0, E.lines[E.cursorY].length - 1);
      break;
  }
}

void handleInsertMode(int c) {
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
    
    default:
      if (c == '\t' || !iscntrl(c))
        editorInsertChar(c);
      break;
  }
}

