#include <editor.h>
#include <fileio.h>
#include <finder.h>
#include <input.h>
#include <keystrokes.h>
#include <lines.h>
#include <output.h>
#include <terminal.h>
#include <tools.h>

void handleDigitCommands(int c) {
  long num = c - 48; 
  int digits = 1;

  while (true) {
    c = editorReadKey();
    if (!isdigit(c)) break;

    if (digits < 10) {
      num = 10 * num + c - 48;
      digits++;
    }
  }
  
  switch (c) {
    case 'k':
      moveCursorToLine(max(1, E.cursorY + 1 - num));
      break;

    case 'j':
      moveCursorToLine(min(E.numlines, E.cursorY + 1 + num));
      break;
      
    case 'G':
      moveCursorToLine(num);
      break;

    case 'g': {
      switch (editorReadKey()) {
        case 'g':
          moveCursorToLine(num);
          break;
      }
      break;
    }
  }
}

void handleNormalMode(int c) {
  if (isdigit(c)) {
    handleDigitCommands(c);
    return; 
  }

  switch (c) {
    // Insert before the cursor
    case 'i':
      E.mode = INSERT;
      break;

    // Insert at the beginning of the line
    case 'I': {
      E.mode = INSERT;
      E.cursorX = indentation(&E.lines[E.cursorY]);
      E.highestLastX = E.cursorX;
      break;
    }

    // Insert after the cursor
    case 'a':
      E.mode = INSERT;
      if (E.cursorX != 0) {
        E.highestLastX = ++E.cursorX;
      }
      break;

    // Insert at the end of the line
    case 'A':
      E.mode = INSERT;
      E.cursorX = E.lines[E.cursorY].length;
      E.highestLastX = E.cursorX;
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
          moveCursorToLine(1);
          break;
      }
      break;
    }

    case 'z': {
      switch (editorReadKey()) {
        // Position cursor on top of the screen
        case 't':
          E.rowOffset = E.cursorY;
          break;

        // Center cursor on screen
        case 'z':
          E.rowOffset = max(0, E.cursorY - E.screenRows / 2);
          break;

        // Position cursor on bottom of the screen
        case 'b':
          E.rowOffset = max(0, E.cursorY - E.screenRows);
          break;
      }
      break;
    }

    // Go to the last line of the document
    case 'G':
      moveCursorToLine(E.numlines);
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

