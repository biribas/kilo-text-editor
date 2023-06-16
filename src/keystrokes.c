#include <editor.h>
#include <fileio.h>
#include <finder.h>
#include <input.h>
#include <keystrokes.h>
#include <lines.h>
#include <output.h>
#include <terminal.h>
#include <tools.h>

// Handle motions 'w', 'W', 'ge' and 'gE'
void handleOuterBoundsHorizontalMotions(bool punctuation, bool fowards) {
  editorLine curLine = E.lines[E.cursorY];
  short direction = fowards ? 1 : -1;

  int x = E.cursorX + (curLine.length ? direction : 0);
  int y = E.cursorY;

  int xLimit;
  int yLimit;

  if (fowards) {
    xLimit = curLine.length;
    yLimit = E.numlines;
  }
  else {
    xLimit = yLimit = -1;
  }

  char cur = curLine.content[E.cursorX];
  bool isFirstCharSpecial = isSpecial(cur);
  bool found = false;

  for (; y != yLimit; y += direction) {
    // When another line is reached
    if (y != E.cursorY) {

      // Empty line
      if (E.lines[y].length == 0) {
        E.cursorY = y;
        E.cursorX = E.highestLastX = 0;
        return;
      }

      // Ignore character type
      found = true;

      if (fowards) {
        x = 0;
        xLimit = E.lines[y].length;
      }
      else {
        x = E.lines[y].length - 1;
        xLimit = -1;
      }
    }

    for (; x != xLimit; x += direction) {
      cur = E.lines[y].content[x];

      if (!isspace(cur) && (found || (punctuation && isFirstCharSpecial ^ isSpecial(cur)))) {
        E.cursorY = y;
        E.cursorX = E.highestLastX = x;
        return;
      }

      if (isspace(cur)) {
        found = true;
        continue;
      }
    }
  }

  E.cursorY = y - direction;
  E.cursorX = E.highestLastX = x - direction;
}

// Handle motions 'e', 'E', 'b' and 'B'
void handleInnerBoundsHorizontalMotions(bool punctuation, bool fowards) {
  editorLine curLine = E.lines[E.cursorY];
  short direction = fowards ? 1 : -1; 

  int x = E.cursorX + (curLine.length ? direction : 0);
  int y = E.cursorY;

  int xLimit;
  int yLimit;

  if (fowards) {
    xLimit = curLine.length;
    yLimit = E.numlines;
  }
  else {
    xLimit = yLimit = -1;
  }

  bool atTheEdgeOfLine = x == xLimit;

  char curChar = curLine.content[E.cursorX];
  char nextChar = atTheEdgeOfLine ? curChar : curLine.content[x];

  bool isFirstCharSpecial = isSpecial(curChar);

  bool searchNextWord = (
    atTheEdgeOfLine ||
    isspace(curChar) ||
    isspace(nextChar) ||
    (punctuation && isFirstCharSpecial ^ isSpecial(nextChar))
  );

  for (; y != yLimit; y += direction) {
    if (y != E.cursorY) {
      if (fowards) {
        x = 0;
        xLimit = E.lines[y].length;
      }
      else {
        x = E.lines[y].length - 1;
        xLimit = -1;
      }
    }

    for (; x != xLimit; x += direction) {
      curChar = E.lines[y].content[x];

      if (searchNextWord) {
        if (!isspace(curChar)) {
          searchNextWord = false;
          isFirstCharSpecial = isSpecial(curChar);
        }
        continue;
      }

      if (isspace(curChar) || (punctuation && (isFirstCharSpecial ^ isSpecial(curChar)))) {
        E.cursorY = y;
        E.cursorX = E.highestLastX = x - direction;
        return;
      }
    }

    if (!searchNextWord) {
      y += direction;
      break;
    }
  }

  E.cursorY = y - direction;
  E.cursorX = E.highestLastX = x - direction;
}

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
      if (E.lines[E.cursorY].length != 0) {
        E.cursorX++;
      }
      E.highestLastX = E.cursorX;
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
      E.cursorX = E.highestLastX = tabs;
      break;
    }

    case 'x': // delete character
    case 's': // delete character and substitute text
      if (E.lines[E.cursorY].length == 0) break;
      if (c == 's') E.mode = INSERT;
      editorLineDeleteChar(&E.lines[E.cursorY], E.cursorX);
      break;

    case 'g': {
      c = editorReadKey();
      switch (c) {
        // Go to the first line of the document
        case 'g':
          moveCursorToLine(1);
          break;

        case 'e': // Jump backwards to the end of a word 
        case 'E': // Jump backwards to the end of a word (words can contain punctuation) 
          handleOuterBoundsHorizontalMotions(islower(c), false);
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

    case 'w': // Jump forwards to the start of a word
    case 'W': // Jump forwards to the start of a word (words can contain punctuation)
      handleOuterBoundsHorizontalMotions(islower(c), true);
      break;

    case 'e': // Jump forwards to the end of a word
    case 'E': // Jump forwards to the end of a word (words can contain punctuation) 
      handleInnerBoundsHorizontalMotions(islower(c), true);
      break;

    case 'b': // Jump backwards to the start of a word
    case 'B': // Jump backwards to the start of a word (words can contain punctuation)
      handleInnerBoundsHorizontalMotions(islower(c), false);
      break;

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
    case '\r':
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
      if (E.cursorX != 0) {
        E.cursorX--;
      }
      E.highestLastX = E.cursorX;
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

